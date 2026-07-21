#include <ui/utils/menu.h>

TuiMenuOption::TuiMenuOption(std::string text, void (*click)(void*)){
	title = text;
	onClick = click;
	data = nullptr;
}

TuiMenu::TuiMenu(std::string text, WINDOW* win){
	title = text;
	window = win;
	is_my_window = 0;
	keypad(window, 1);
}
TuiMenu::TuiMenu(std::string text, int yb, int xb, int ys, int xs){
	title = text;
	change_window(yb, xb, ys, xs);
}

TuiMenu::~TuiMenu(){
	delete_window();
}

void TuiMenu::delete_window(){
	if(window == nullptr) return;
	wclear(window);
	if(is_my_window){
		delwin(window);
	}
	window = nullptr;
	is_my_window = 0;
	refresh();
}
void TuiMenu::change_window(int yb, int xb, int ys, int xs){
	delete_window();
	is_my_window = 1;
	window = newwin(yb, xb, ys, xs);
	nodelay(window, 0);
	keypad(window, 1);
	immedok(window, 0);
	clearok(window, 0);
	refresh();
	wrefresh(window);
}

void TuiMenu::add_option(TuiMenuOption tmo){
	option.push_back(tmo);
}
void TuiMenu::delete_option_index(int index){
	if(index < 0 || index >= option.size())
		return ;
	option.erase(option.begin() + index);
}
void TuiMenu::delete_option_title(std::string title ){
	for(int i=0 ; i < option.size() ; i++){
		if(option[i].title == title){
			delete_option_index(i);
			return ;
		}
	}
}

size_t TuiMenu::option_count(){
	return option.size();
}
void TuiMenu::fix_size(){
	int mx = 1;
	for(int i=0 ; i < option.size(); i++)
		mx = std::max(mx, (int)option[i].title.size());
	
	change_window(option.size()+2,
			mx + 3 + std::max(selected_item_prefix.size(), not_selected_item_prefix.size()),
			getbegy(window), getbegx(window)
		);
}

void TuiMenu::clear(){
	if(window == nullptr) return;
	wclear(window);
	wrefresh(window);
	refresh();
}
int TuiMenu::get_maxy(){ return getmaxy(window); }
int TuiMenu::get_maxx(){ return getmaxx(window); }
int TuiMenu::get_begy(){ return getbegy(window); }
int TuiMenu::get_begx(){ return getbegx(window); }

void TuiMenu::center(int ysize, int xsize, int ybeg, int xbeg){
	int ymax = get_maxy(), xmax = get_maxx(),
		yb = ( (ysize - ymax) / 2) + ybeg ,
		xb = ( (xsize - xmax) / 2 ) + xbeg;
	change_window(ymax, xmax, yb, xb);
}
void TuiMenu::center(WINDOW* win){
	center(getmaxy(win), getmaxx(win), getbegy(win), getbegx(win));
}
bool TuiMenu::is_active(){
	return isactive;
}
void TuiMenu::enable(){
	isactive = 1;
	display();
}
void TuiMenu::disable(){
	printw("in disable ");
	getch();
	this->clear();
	printw("after disable ");
	getch();
	isactive = 0;
}
void TuiMenu::display(){
	box_color.enable(window);
	box(window, 0, 0);
	box_color.disable(window);
	
	int max_options = get_maxx() - 2;
	int start_show = std::max(0, selected_item - (max_options / 2));
	
	for(int i=0 ; i < std::min( max_options, (int)option.size() ) ; i++){
		ScreenColor* col = (ScreenColor*)( (i == selected_item) ? &selected_item_color : &not_selected_item_color );
		
		mvwprintw( window, i+1 , 1,
				( (std::string)( (i == selected_item) ? selected_item_prefix : not_selected_item_prefix  )
					+  option[i].title).c_str()
				);
		
		mvwchgat(window, i+1, 1, getmaxx(window) - 2, col->get_attrs(), col->get_pair_id(), NULL );
	}
	wrefresh(window);
	refresh();
}
void TuiMenu::handle_input(int input){
	printw("handle input(%d) ", input);
	switch(input){
		case KEY_DOWN:
			selected_item += 1;
			selected_item %= option.size();
			break;
		case KEY_UP:
			selected_item += option.size() - 1;
			selected_item %= option.size();
			break;
		case '\n':
			if(selected_item < 0 || selected_item >= option.size()){
				break;
			}
			printw("before if ");
			if(option[selected_item].onClick != nullptr){
				printw("try to open ");
				option[selected_item].onClick(this);
			}
			break;
	}
	display();
}

void TuiMenu::run(){
	enable();
	keypad(window, 1);
	int input;
	while(is_active()){
		input = wgetch(window);
		if(input == 'q')
			break;
		handle_input(input);
	}
}

void tui_menu_option_exit_function(void* tuimenu){
	TuiMenu* menu = (TuiMenu*)tuimenu;
	menu->disable();
	
}

