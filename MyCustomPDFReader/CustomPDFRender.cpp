#include"CustomPDFReader.hpp"

void Reader::getPathFile() {
    auto openFileDialog = tgui::FileDialog::create("Apri file", "Apri");
    this->gui->add(openFileDialog);
    
    openFileDialog->onFileSelect([this, openFileDialog](const std::vector<tgui::Filesystem::Path>& paths) {
        if (!paths.empty()) {
            std::string nuovoPath = paths[0].asString().toStdString();
            if (this->document)
                delete this->document;
            this->document = new Document(nuovoPath.c_str());
            this->document->loadPage(1, this->win);
            this->gui->get<tgui::Label>("pages")->setText(tgui::String(std::format("{}/{}", 1, this->document->getTotalPages())));
        }
    });

    openFileDialog->onClose([this,openFileDialog] {
        this->gui->remove(openFileDialog); // Rimuove il dialogo
    });
}

void Reader::about() {
    MessageBox(NULL,L"Questo programma e' stato creato da ninuzzomagno.\nLibrerie: mupdf, sfml, tgui, curl.\nhttps://github.com/ninuzzomagno", L"Info", MB_OK);
}

Reader::Reader() {
	this->win = new sf::RenderWindow(sf::VideoMode({ 1920, 1080 }), "PDF reader", sf::Style::Default);
	this->gui = new tgui::Gui(*this->win);
    this->document = nullptr;
    this->from = "en";
    this->to = "it";
}

void Reader::setupGui() {
    auto menu = tgui::MenuBar::create();
    menu->setTextSize(20);
    menu->setPosition(0,0);
    menu->setSize("100%", 25);
    menu->addMenu("File");
    menu->addMenuItem("Apri");
    menu->connectMenuItem({ "File","Apri" }, [this]() {this->getPathFile(); });
    menu->addMenu("Traduzione");

    menu->addMenuItem({ "Traduzione","Da","Inglese" });
    menu->addMenuItem({ "Traduzione","Da","Spagnolo" });
    menu->addMenuItem({ "Traduzione","Da","Italiano" });
    menu->addMenuItem({ "Traduzione","Da","Tedesco" });
    menu->addMenuItem({ "Traduzione","Da","Francese" });

    menu->addMenuItem({ "Traduzione","A","Inglese" });
    menu->addMenuItem({ "Traduzione","A","Spagnolo" });
    menu->addMenuItem({ "Traduzione","A","Italiano" });
    menu->addMenuItem({ "Traduzione","A","Tedesco" });
    menu->addMenuItem({ "Traduzione","A","Francese" });

    menu->connectMenuItem({ "Traduzione", "Da", "Inglese" }, [this]() { this->from = "en"; });
    menu->connectMenuItem({ "Traduzione", "Da", "Italiano" }, [this]() { this->from = "it"; });
    menu->connectMenuItem({ "Traduzione", "Da", "Spagnolo" }, [this]() { this->from = "es"; });
    menu->connectMenuItem({ "Traduzione", "Da", "Tedesco" }, [this]() { this->from = "de"; });
    menu->connectMenuItem({ "Traduzione", "Da", "Francese" }, [this]() { this->from = "fr"; });

    menu->connectMenuItem({ "Traduzione", "A", "Italiano" }, [this]() { this->to = "it"; });
    menu->connectMenuItem({ "Traduzione", "A", "Inglese" }, [this]() { this->to = "en"; });
    menu->connectMenuItem({ "Traduzione", "A", "Tedesco" }, [this]() { this->to = "de"; });
    menu->connectMenuItem({ "Traduzione", "A", "Spagnolo" }, [this]() { this->to = "es"; });
    menu->connectMenuItem({ "Traduzione", "A", "Francese" }, [this]() { this->to = "fr"; });

    menu->addMenu("Info");
    menu->connectMenuItem({ "Info" }, [this]() {this->about(); });
    this->gui->add(menu, "menu");

    auto sidebar = tgui::Group::create();
    sidebar->setPosition("100% - 100", 25);
    sidebar->setSize(100, "100% - 25");
    this->gui->add(sidebar, "Sidebar");

    auto label = tgui::Label::create("0/0");
    label->setTextSize(20);
    label->setPosition(5, 10);
    sidebar->add(label,"pages");

    auto traduzione_label = tgui::Label::create("");
    traduzione_label->setTextSize(30);
    traduzione_label->setSize("100%-45", 100);
    traduzione_label->setPosition(0, "100%-100");
    this->gui->add(traduzione_label, "traduzione");
}

void Reader::mainloop() {

    this->setupGui();

    fz_rect selection_rect{ 0,0,0,0 };
        
    while (this->win->isOpen()) {

        while (const std::optional event = this->win->pollEvent())
        {
            this->gui->handleEvent(*event);

            if (event->is<sf::Event::Closed>())
                this->win->close();
            if (this->document) {
                if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*this->win);
                    if (mouse_pos.x < this->win->getSize().x - 45 && mouse_pos.y>25 && mouse_pos.y < this->win->getSize().y - 100)
                        if (mouseWheel->wheel == sf::Mouse::Wheel::Vertical)
                            this->document->scroll(-mouseWheel->delta * 30.f, this->win);
                }
                else if (event->is<sf::Event::MouseButtonPressed>()) {
                    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*this->win);
                    selection_rect.x0 = mouse_pos.x;
                    selection_rect.y0 = mouse_pos.y;
                }
                else if (event->is<sf::Event::MouseButtonReleased>()) {
                    sf::Vector2i mouse_pos = sf::Mouse::getPosition(*this->win);
                    selection_rect.x1 = mouse_pos.x;
                    selection_rect.y1 = mouse_pos.y;

                    if (selection_rect.x1 < selection_rect.x0) {
                        float v = selection_rect.x0;
                        selection_rect.x0 = selection_rect.x1;
                        selection_rect.x1 = v;
                    }

                    if (selection_rect.y1 < selection_rect.y0) {
                        float v = selection_rect.y0;
                        selection_rect.y0 = selection_rect.y1;
                        selection_rect.y1 = v;
                    }


                    std::string p = this->document->findWords(selection_rect);
                    std::cout << "PAROLA TROVATA: " << p << std::endl;

                    this->gui->get<tgui::Label>("traduzione")->setText(this->document->translate(p, this->from, this->to));
                }
                else if (event->is<sf::Event::KeyPressed>()) {

                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
                        this->gui->get<tgui::MenuBar>("menu")->getRenderer()->setBackgroundColor(this->document->isInverted() ? sf::Color::White : sf::Color::Black);
                        this->gui->get<tgui::MenuBar>("menu")->getRenderer()->setTextColor(this->document->isInverted() ? sf::Color::Black : sf::Color::White);
                        this->gui->get<tgui::Label>("pages")->getRenderer()->setTextColor(this->document->isInverted() ? sf::Color::Black : sf::Color::White);
                        this->gui->get<tgui::Label>("traduzione")->getRenderer()->setTextColor(this->document->isInverted() ? sf::Color::Black : sf::Color::White);
                        this->document->invertColors(this->win);
                    }
                    else {
                        int page = 0;
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
                            page = this->document->loadNextPage(win);
                        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
                            page = this->document->loadPreviousPage(win);

                        this->gui->get<tgui::Label>("pages")->setText(tgui::String(std::format("{}/{}", page, this->document->getTotalPages())));
                    }
                }
            }
        }


        if (!this->document)
            this->win->clear(sf::Color::White);
        else {
            this->win->clear(this->document->isInverted() ? sf::Color::Black : sf::Color::White);
            this->document->render(this->win);
        }

        this->gui->draw();
        this->win->display();
    }
}

Reader::~Reader() {
    delete this->gui;
    delete this->win;
}