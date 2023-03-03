#pragma once

#include "imgui.h"

#include <cassert>

#include <string>
#include <optional>
#include <utility>

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

	bool chkbox(const char *lbl, bool &b) {
		return ImGui::Checkbox(lbl, &b);
	}
};

class MainMenuBar final {
	bool open;
public:
	MainMenuBar() : open(ImGui::BeginMainMenuBar()) {}
	MainMenuBar(const MainMenuBar&) = delete;

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
