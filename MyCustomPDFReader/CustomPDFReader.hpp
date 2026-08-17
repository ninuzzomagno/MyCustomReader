#pragma once

#include"TGUI/TGUI.hpp"
#include <TGUI/Widgets/MessageBox.hpp>
#include"TGUI/Backend/SFML-Graphics.hpp"
#include"SFML/Graphics.hpp"
#include"mupdf/fitz.h"
#include"mupdf/pdf.h"

#include"nlohmann/json.hpp"

#include<format>
#include<iostream>
#include<string>
#include<sstream>
#include<curl/curl.h>

#include<algorithm>

#pragma comment(lib,"libcurl")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Normaliz.lib")

#define DEBUG

#ifdef DEBUG

#pragma comment(lib,"sfml-graphics-d")
#pragma comment(lib,"sfml-window-d")
#pragma comment(lib,"sfml-system-d")
#pragma comment(lib,"tgui-d")

#else

#pragma comment(lib,"sfml-graphics")
#pragma comment(lib,"sfml-window")
#pragma comment(lib,"sfml-system")
#pragma comment(lib,"tgui")

#endif


#pragma comment(lib,"libmupdf")
#pragma comment(lib,"libthirdparty")

size_t WriteCb(void* contents, size_t size, size_t nmemb, std::string* s);

class Document {
public:
	Document(const char* path);
	inline void setZoom(float z) { this->zoom = z; };
	inline void setRotate(float r) { this->rotate = r; };
	inline int getTotalPages() { return this->num_pages; };
	inline bool isInverted() { return this->inverted; };

	std::string findWords(fz_rect&);

	void invertColors(sf::RenderWindow* win);

	int loadNextPage(sf::RenderWindow*);
	int loadPreviousPage(sf::RenderWindow*);

	void loadPage(int, sf::RenderWindow*);

	void scroll(float, sf::RenderWindow*);
	std::string translate(std::string& data, std::string& from, std::string& to);
	void render(sf::RenderWindow*);

private:
	bool inverted;

	int current_page, num_pages;

	fz_context* ctx;
	fz_document* doc;
	fz_pixmap* pix;
	fz_matrix ctm;

	fz_stext_page* stext_page;

	float zoom, rotate;
	int w, h, scrollY;

	sf::Texture* texture;
	sf::Sprite* sprite;
};

class Reader {
public:
	Reader();
	void mainloop();
	
	~Reader();
private:
	void getPathFile();
	void about();
	void setupGui();
	Document* document;
	sf::RenderWindow* win;
	tgui::Gui* gui;

	std::string from, to;

	std::string filepath;
};

