#include"CustomPDFReader.hpp"

using json = nlohmann::json;

Document::Document(const char* path) {
	this->current_page = 1;
	this->num_pages = -1;
	this->zoom = 100.f;
	this->rotate = 0.f;
	this->scrollY = 0;

	this->inverted = false;

	std::cout << "FILE: " << path << std::endl;

	this->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
	if (!this->ctx) {
		std::cout << "Impossibile creare il context" << std::endl;
		exit(EXIT_FAILURE);
	}

	fz_try(this->ctx)
		fz_register_document_handlers(this->ctx);
	fz_catch(this->ctx) {
		fz_report_error(this->ctx);
		std::cout << "Impossibile registrare il document handlers" << std::endl;
		fz_drop_context(this->ctx);
		exit(EXIT_FAILURE);
	}

	fz_try(this->ctx)
		this->doc = fz_open_document(this->ctx, path);
	fz_catch(this->ctx) {
		fz_report_error(this->ctx);
		std::cout << "Impossibile aprire il documento " << path << std::endl;
		fz_drop_context(this->ctx);
		exit(EXIT_FAILURE);
	}

	fz_try(this->ctx)
		this->num_pages = fz_count_pages(this->ctx, this->doc);
	fz_catch(this->ctx) {
		fz_report_error(this->ctx);
		std::cout << "Impossibile calcolare il numero di pagine" << std::endl;
		fz_drop_document(this->ctx, this->doc);
		fz_drop_context(this->ctx);
		exit(EXIT_FAILURE);
	}

	std::cout << "Total pages: " << this->num_pages << std::endl;

	this->texture = new sf::Texture;
	sf::Vector2u viewSize = { (unsigned int)1920 - 100, (unsigned int)1080 - 125 };
	this->texture->resize(viewSize);
	this->sprite = new sf::Sprite(*this->texture);
}

int Document::loadNextPage(sf::RenderWindow* win) {
	if (this->current_page < this->num_pages)
		this->loadPage(this->current_page + 1, win);
	return this->current_page+1;
}

int Document::loadPreviousPage(sf::RenderWindow* win) {
	if (this->current_page > 0)
		this->loadPage(this->current_page - 1, win);
	return this->current_page+1;
}

void Document::invertColors(sf::RenderWindow*win) {
	this->inverted = !this->inverted;
	this->loadPage(this->current_page, win);
}

void Document::loadPage(int page,sf::RenderWindow*win) {
	if (page < 0)
		page = 0;
	else if (page == this->num_pages)
		page = this->num_pages-1;

	if (this->stext_page)
		fz_drop_stext_page(this->ctx, this->stext_page);
	if(this->pix)
		fz_drop_pixmap(this->ctx, this->pix);

	fz_page* page_ptr = fz_load_page(this->ctx, this->doc, page);
	fz_rect rect = fz_bound_page(this->ctx, page_ptr);

	float pageWidthPoints = rect.x1 - rect.x0;

	this->zoom = static_cast<float>(win->getSize().x-100) / pageWidthPoints * 100.f;
	this->ctm = fz_scale(this->zoom / 100, this->zoom / 100);
	this->ctm = fz_pre_rotate(this->ctm, this->rotate);

	std::cout << "Zoom: " << this->zoom << std::endl;

	fz_stext_options options = { FZ_STEXT_PRESERVE_WHITESPACE };
	this->stext_page = fz_new_stext_page_from_page(this->ctx, page_ptr, &options);

	fz_drop_page(this->ctx, page_ptr);

	fz_try(this->ctx)
		this->pix = fz_new_pixmap_from_page_number(this->ctx, this->doc, page, this->ctm, fz_device_rgb(this->ctx), 1);
	fz_catch(this->ctx) {
		fz_report_error(this->ctx);
		std::cout << "Impossibile renderizzare la pagina " << page << std::endl;
		fz_drop_document(this->ctx, this->doc);
		fz_drop_context(this->ctx);
		exit(EXIT_FAILURE);
	}

	this->current_page = page;
	
	this->w = (unsigned int)fz_pixmap_width(this->ctx, this->pix);
	this->h = (unsigned int)fz_pixmap_height(this->ctx, this->pix);
	
	std::cout << "W: " << w << "\tH: " << h << std::endl;

	if (this->inverted)
		fz_invert_pixmap(this->ctx, this->pix);

	this->texture->update(this->pix->samples);
	this->sprite->setTexture(*this->texture, true);
}

void Document::render(sf::RenderWindow*win) {
	win->draw(*this->sprite);
}

void Document::scroll(float delta,sf::RenderWindow*win) {
	this->scrollY += delta;
	std::cout << this->texture->getSize().x << "\t" << this->texture->getSize().y << std::endl;
	if (this->scrollY < 0)this->scrollY = 0;
	float texture_H = static_cast<float>(this->texture->getSize().y);
	if (this->scrollY > this->h - texture_H)
		this->scrollY = this->h - texture_H;

	unsigned char* sourcePtr = this->pix->samples + (this->scrollY * this->w * 4);
	this->texture->update(sourcePtr);
}

std::string Document::findWords(fz_rect&rect) {
	fz_matrix inv = fz_invert_matrix(this->ctm);
	rect.y0 += this->scrollY;
	rect.y1 += this->scrollY;
	rect = fz_transform_rect(rect, inv);

	char* testo = fz_copy_rectangle(this->ctx, this->stext_page, rect, 0);
	std::string ris = "";
	if (testo) {
		ris = testo;
		fz_free(this->ctx, testo);
		std::replace(ris.begin(), ris.end(), '\n', ' ');
		std::replace(ris.begin(), ris.end(), '\r', ' ');
	}
	return ris;
}

std::string Document::translate(std::string& data, std::string& from, std::string& to) {
	CURL* curl = curl_easy_init();
	std::string response;
	if (curl) {
		char* escapedText = curl_easy_escape(curl, data.c_str(), (int)data.length());
		
		std::string url = std::format("https://translate.googleapis.com/translate_a/single?client=gtx&sl={}&tl={}&dt=t&q=",from,to);
		url += escapedText;
		
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

		CURLcode res = curl_easy_perform(curl);

		curl_easy_cleanup(curl);

		if (res == CURLE_OK) {
			try {
				auto j = json::parse(response);

				std::string testoTradotto = "";

				if (j.is_array() && !j[0].is_null()) {
					for (auto& element : j[0]) {
						if (element.is_array() && element[0].is_string()) {
							testoTradotto += element[0].get<std::string>();
						}
					}
				}
				return testoTradotto;
			}
			catch (json::parse_error& e) {
				std::cout << "Errore parsing json: " << e.what() << std::endl;
			}
		}
	}
	return "Errore traduzione";
}

size_t WriteCb(void* contents, size_t size, size_t nmemb, std::string* s) {
	size_t newLength = size * nmemb;
	s->append((char*)contents, newLength);
	return newLength;
}