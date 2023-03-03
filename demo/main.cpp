// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl_opengl3/ folder**
// See imgui_impl_sdl.cpp for details.

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "net.hpp"

#include <cassert>
#include <cstdint>

#include <array>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdio>

#include "imfilebrowser.h"

class Menu final {
	bool open;
public:
	const std::string lbl;

	Menu(const char *lbl, bool open) : open(open), lbl(lbl) {}
	Menu(const Menu&) = delete;
	Menu(Menu &&m) noexcept : open(std::exchange(m.open, false)), lbl(lbl) {}

	~Menu() {
		if (open)
			ImGui::EndMenu();
	}

	constexpr operator bool() const noexcept { return open; }

	bool item(const char *lbl) {
		assert(open);
		return ImGui::MenuItem(lbl);
	}
};

class MainMenuBar final {
	bool open;
public:
	MainMenuBar() : open(ImGui::BeginMainMenuBar()) {}

	~MainMenuBar() {
		if (open)
			ImGui::EndMainMenuBar();
	}

	constexpr operator bool() const noexcept { return open; }

	std::optional<Menu> menu(const char *lbl) {
		if (!ImGui::BeginMenu(lbl))
			return std::nullopt;

		return Menu(lbl, true);
	}
};

class Frame final {
	bool open;
public:
	const std::string lbl;

	Frame(const char *lbl) : open(ImGui::Begin(lbl)), lbl(lbl) {}
	Frame(const Frame&) = delete;
	Frame(Frame &&f) noexcept : open(std::exchange(f.open, false)), lbl(f.lbl) {}

	~Frame() {
		ImGui::End();
	}

	constexpr operator bool() const noexcept { return open; }

	bool btn(const char *lbl) {
		return ImGui::Button(lbl);
	}

	void sl() { ImGui::SameLine(); }
};

class MOS6510 final {
public:
	uint8_t acc;
	uint8_t y;
	uint8_t x;
	uint16_t pc;
	uint8_t sp;

	unsigned carry : 1;
	unsigned zero : 1;
	unsigned no_irq : 1;
	unsigned dec : 1;
	unsigned brk : 1;
	unsigned of : 1;
	unsigned neg : 1;

	unsigned psw() const noexcept {
		unsigned v = 0;

		if (carry) v |= 1 << 0;
		if (zero) v |= 1 << 1;
		if (no_irq) v |= 1 << 2;
		if (dec) v |= 1 << 3;
		if (brk) v |= 1 << 4;
		v |= (1 << 5);
		if (of) v |= 1 << 6;
		if (neg) v |= 1 << 7;

		return v;
	}
};

class U1541;

class VIC final {
public:
	uint8_t border;
	uint8_t background;
	int combo_bank, combo_charrom, combo_scrptr;
	std::vector<std::string> charrom_items, scrptr_items;
	U1541 &c64;
	bool autoreset;

	VIC(U1541 &c64) : border(14), background(6), combo_bank(0), combo_charrom(2), combo_scrptr(1), charrom_items(), scrptr_items(), c64(c64), autoreset(true) {
		load_memsetup();
	}

	void reset();

	void show();
private:
	void change_bank(int i);
	void load_memsetup();
	void store_memsetup();
	void show_col(Frame&, const char *lbl, uint16_t addr, uint8_t &v);
};

class U1541 final {
	char buf_ip[32];
	uint16_t ip_port;
	uint16_t poke_addr;
	uint8_t poke_val;
	bool autopoke;
	std::unique_ptr<TcpSocket> sock;
	std::vector<uint8_t> data;
	char keybuf[6];

	VIC vic;
	ImGui::FileBrowser fb_prg;
	std::string prg_path;
	std::vector<uint8_t> prg_data;
public:
	static constexpr unsigned min_prg_size = 2;
	static constexpr unsigned max_prg_size = min_prg_size + 52 * 1024;

	U1541() : buf_ip("192.168.2.64"), ip_port(64), poke_addr(0xd020), poke_val(0), autopoke(false), sock(), data(), keybuf(), vic(*this), fb_prg(), prg_path(), prg_data() {}

	void show();
	void show_connected(Frame&);

	void show_prg_control();

	void poke(uint16_t addr, uint8_t v);
	void kbp(const char *str);

	void load_prg(const std::string&);
	void send_prg(const std::vector<uint8_t>&);
};

class Dissassembler final {
	uint8_t op;
	uint8_t v1, v2;
public:
	void show();
};

class Engine final {
	MOS6510 mpu;
	Net net;
	U1541 u1541;
	Dissassembler diss;
public:
	Engine() : mpu(), net(), u1541(), diss() {}

	void display();
	void show_menubar();
	void show_mpu();
};

void Engine::show_menubar() {
	MainMenuBar mmb;
	if (!mmb)
		return;

	#if 0
	auto m = mmb.menu("File");
	if (m) {
		if (m->item("Quit"))
			throw 0;
	}
	#endif
}

void Dissassembler::show() {
	Frame f("Dissassembler");
	if (!f)
		return;

	uint8_t step = 1;

	ImGui::InputScalar("Opcode", ImGuiDataType_U8, &op, &step, NULL, "%02X");

	switch (op) {
	case 0x00:
		ImGui::TextUnformatted("BRK 7");
		break;
	case 0x01:
		ImGui::TextUnformatted("ORA izx 6");
		ImGui::InputScalar("ZP index", ImGuiDataType_U8, &v1, &step, NULL, "%02X");
		ImGui::Text("ORA ($%02X,X)", v1);
		break;
	case 0x02:
	case 0x12:
	case 0x22:
	case 0x32:
	case 0x42:
	case 0x52:
	case 0x62:
	case 0x72:
		ImGui::TextUnformatted("KIL");
		ImGui::TextWrapped("%s", "this opcode will lock up the CPU");
		break;
	default:
		ImGui::TextUnformatted("unknown opcode");
		break;
	}
}

std::array<std::string, 16> vic_colors{ "black", "white", "red", "cyan", "magenta", "green", "blue", "yellow", "orange", "brown", "purple", "dark gray", "medium gray", "light green", "light blue", "light gray"};

std::array<const char*, 4> vic_banks{ "$0000-$3FFF", "$4000-$7FFF", "$8000-$BFFF", "$C000-$FFFF" };

bool combo_vec(void *data, int idx, const char **out_text) {
	std::vector<std::string> &vec = *(std::vector<std::string>*)data;

	if (idx < 0 || idx >= vec.size())
		return false;

	*out_text = vec.at(idx).c_str();
	return true;
}

void VIC::change_bank(int i) {
	combo_bank = std::clamp<int>(i, 0, vic_banks.size() - 1);
	int bank = 3 - combo_bank;

	c64.poke(0xdd00, bank & 0x3);
}

void VIC::reset() {
	if (!autoreset)
		return;

	border = 14;
	background = 6;
	combo_bank = 0;
	combo_charrom = 2;
	combo_scrptr = 1;
	load_memsetup();
}

void VIC::show() {
	Frame f("VIC");

	if (!f)
		return;

	show_col(f, "Border color", 0xd020, border);
	show_col(f, "Background color", 0xd021, background);

	uint8_t step = 1;

	if (ImGui::Combo("Bank", &combo_bank, vic_banks.data(), vic_banks.size())) {
		change_bank(combo_bank);
		load_memsetup();
	}

	if (ImGui::Combo("Character memory", &combo_charrom, combo_vec, &charrom_items, charrom_items.size())) {
		store_memsetup();
	}

	if (ImGui::Combo("Screen pointer", &combo_scrptr, combo_vec, &scrptr_items, scrptr_items.size())) {
		store_memsetup();
	}

	ImGui::Checkbox("Reset on C64 Reset", &autoreset);
	f.sl();

	if (f.btn("Poke all values")) {
		c64.poke(0xd020, border);
		c64.poke(0xd021, background);
		store_memsetup();
	}
}

void VIC::store_memsetup() {
	combo_charrom = std::clamp<int>(combo_charrom, 0, charrom_items.size());
	combo_scrptr = std::clamp<int>(combo_scrptr, 0, scrptr_items.size());

	uint8_t v = 0;

	v |= ((unsigned)combo_charrom) << 1u;
	v |= ((unsigned)combo_scrptr) << 4u;

	c64.poke(0xd018, v);
}

void VIC::load_memsetup() {
	uint16_t base = (unsigned)combo_bank * 0x4000;

	charrom_items.clear();
	scrptr_items.clear();

	char buf[32];

	for (unsigned i = 0; i < 8; ++i) {
		snprintf(buf, sizeof buf, "$%04X-$%04X", base + 0x800 * i, base + 0x800 * (i + 1) - 1);
		charrom_items.emplace_back(buf);
	}

	for (unsigned i = 0; i < 16; ++i) {
		snprintf(buf, sizeof buf, "$%04X-$%04X", base + 0x400 * i, base + 0x400 * (i + 1) - 1);
		scrptr_items.emplace_back(buf);
	}
}

void VIC::show_col(Frame &f, const char *lbl, uint16_t addr, uint8_t &v) {
	uint8_t step = 1;
	if (ImGui::InputScalar(lbl, ImGuiDataType_U8, &v, &step, NULL, "%02X"))
		c64.poke(addr, v);

	f.sl();
	ImGui::Text("(%s)", vic_colors[v % vic_colors.size()].c_str());
}

void U1541::load_prg(const std::string &path) {
	//printf("prg path: %s\n", path.c_str());

	try {
		std::ifstream in(path, std::ios::binary);
		in.exceptions(std::ifstream::failbit);

		in.seekg(0, std::ios_base::end);
		size_t end = in.tellg();
		in.seekg(0);
		size_t begin = in.tellg();

		size_t size = end - begin;

		//printf("prg size: %zu\n", size);

		prg_path = path;

		prg_data.resize(size);
		in.read((char*)prg_data.data(), size);
	} catch (const std::runtime_error &e) {
		fprintf(stderr, "%s: %s\n", __func__, e.what());
	}
}

void U1541::show_prg_control() {
	Frame f("PRG control");

	if (!f)
		return;

	if (prg_data.size() > min_prg_size) {
		if (f.btn("Reload PRG"))
			load_prg(prg_path);

		f.sl();

		if (f.btn("Reload and Start PRG")) {
			load_prg(prg_path);
			if (prg_data.size() > min_prg_size)
				send_prg(prg_data);
		}
	}

	if (f.btn("Load PRG"))
		fb_prg.Open();

	fb_prg.Display();

	if (fb_prg.HasSelected()) {
		load_prg(fb_prg.GetSelected().string());
		fb_prg.ClearSelected();
	}

	if (prg_data.size() > min_prg_size) {
		f.sl();

		if (f.btn("Start PRG"))
			send_prg(prg_data);
	}
}

void U1541::show_connected(Frame &f) {
	if (f.btn("Disconnect"))
		sock.reset();

	if (f.btn("Reset")) {
		data.clear();
		data.emplace_back(0xff04 & 0xff);
		data.emplace_back(0xff04 >> 8);
		data.emplace_back(0);
		data.emplace_back(0);

		sock->send_fully(data.data(), data.size());
		vic.reset();
	}

	uint16_t step = 1;
	uint8_t step2 = 1;

	ImGui::InputScalar("Poke address", ImGuiDataType_U16, &poke_addr, &step, NULL, "%04X");

	if (ImGui::InputScalar("Poke value", ImGuiDataType_U8, &poke_val, &step2, NULL, "%02X") && autopoke)
		poke(poke_addr, poke_val);

	if (f.btn("Poke"))
		poke(poke_addr, poke_val);

	f.sl();
	ImGui::Checkbox("Autopoke", &autopoke);

	if (f.btn("DEC"))
		poke(poke_addr, --poke_val);

	f.sl();

	if (f.btn("INC"))
		poke(poke_addr, ++poke_val);

	if (f.btn("A"))
		kbp("A");

	show_prg_control();

	ImGui::InputText("Text", keybuf, sizeof keybuf);
	if (f.btn("Type")) {
		keybuf[(sizeof keybuf) - 1] = '\0';
		kbp(keybuf);
	}

	vic.show();
}

void U1541::show() {
	Frame f("Ultimate 1541 interface");
	if (!f)
		return;

	uint16_t step = 1;
	uint8_t step2 = 1;

	bool connected = sock.get() != nullptr;

	if (connected) ImGui::BeginDisabled();
	ImGui::InputText("IP address", buf_ip, sizeof buf_ip);
	ImGui::InputScalar("IP port", ImGuiDataType_U16, &ip_port, &step);
	if (connected) ImGui::EndDisabled();

	if (connected) {
		show_connected(f);
	} else if (f.btn("Connect")) {
		try {
			sock.reset(new TcpSocket());
			sock->connect(buf_ip, ip_port);
		} catch (const std::runtime_error &e) {
			fprintf(stderr, "%s: %s\n", __func__, e.what());
		}
	}
}

void U1541::poke(uint16_t addr, uint8_t val) {
	data.clear();
	data.emplace_back(0xff06 & 0xff);
	data.emplace_back(0xff06 >> 8);

	unsigned size = 3;

	data.emplace_back(size & 0xff);
	data.emplace_back(size >> 8);

	// 3 bytes
	data.emplace_back(addr & 0xff);
	data.emplace_back(addr >> 8);
	data.emplace_back(val);

	sock->send_fully(data.data(), data.size());
}

void U1541::kbp(const char *str) {
	data.clear();
	data.emplace_back(0xff03 & 0xff);
	data.emplace_back(0xff03 >> 8);

	unsigned size = strlen(str);

	data.emplace_back(size & 0xff);
	data.emplace_back(size >> 8);

	for (unsigned i = 0; i < size; ++i)
		data.emplace_back(str[i]);

	sock->send_fully(data.data(), data.size());
}

void U1541::send_prg(const std::vector<uint8_t> &prg) {
	if (prg.size() >= max_prg_size)
		throw std::runtime_error("prg too big");

	data.clear();
	data.emplace_back(0xff02 & 0xff);
	data.emplace_back(0xff02 >> 8);

	unsigned size = prg.size();

	data.emplace_back(size & 0xff);
	data.emplace_back(size >> 8);

	data.insert(data.end(), prg.begin(), prg.end());

	sock->send_fully(data.data(), data.size());
}

void Engine::show_mpu() {
	Frame f("CPU");
	if (!f)
		return;

	uint8_t step = 1;
	uint16_t step2 = 1;

	ImGui::InputScalar("Accumulator", ImGuiDataType_U8, &mpu.acc, &step, NULL, "%02X");
	ImGui::InputScalar("X index", ImGuiDataType_U8, &mpu.x, &step, NULL, "%02X");
	ImGui::InputScalar("Y index", ImGuiDataType_U8, &mpu.y, &step, NULL, "%02X");
	ImGui::InputScalar("Stack Pointer", ImGuiDataType_U8, &mpu.sp, &step, NULL, "%02X");
	ImGui::InputScalar("Program Counter", ImGuiDataType_U16, &mpu.pc, &step2, NULL, "%04X");

	unsigned psw = mpu.psw();

	if (f.btn(psw & (1 << 7) ? "N" : "n")) mpu.neg ^= 1; f.sl();
	if (f.btn(psw & (1 << 6) ? "V" : "v")) mpu.of ^= 1; f.sl();
	ImGui::TextUnformatted("1"); f.sl();
	if (f.btn(psw & (1 << 4) ? "B" : "b")) mpu.brk ^= 1; f.sl();
	if (f.btn(psw & (1 << 3) ? "D" : "d")) mpu.dec ^= 1; f.sl();
	if (f.btn(psw & (1 << 2) ? "I" : "i")) mpu.no_irq ^= 1; f.sl();
	if (f.btn(psw & (1 << 1) ? "Z" : "z")) mpu.zero ^= 1; f.sl();
	if (f.btn(psw & (1 << 0) ? "C" : "c")) mpu.carry ^= 1; f.sl();

	ImGui::Text("%02X", psw);
}

void Engine::display() {
	show_menubar();
	u1541.show();
	diss.show();
	show_mpu();
}

// Main code
int main(int, char**)
{
	int ret = 1;

	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to the latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window = SDL_CreateWindow("Commodore 64 code monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL2_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	bool show_demo_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	Engine eng;

	try {
		// Main loop
		bool done = false;
		while (!done)
		{
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				ImGui_ImplSDL2_ProcessEvent(&event);
				if (event.type == SDL_QUIT)
					done = true;
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
					done = true;
			}

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

			// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
			{
				ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

				ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
				ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::End();
			}

			eng.display();

			// Rendering
			ImGui::Render();
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(window);
		}
	} catch (int v) {
		ret = v;
	}

	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return ret;
}
