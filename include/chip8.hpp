#pragma once

#include <array>
#include <vector>
#include <limits>

namespace chip8 {

enum class key : size_t {
	key_0,
	key_1,
	key_2,
	key_3,
	key_4,
	key_5,
	key_6,
	key_7,
	key_8,
	key_9,
	key_a,
	key_b,
	key_c,
	key_d,
	key_e,
	key_f,
	key_num,
};

template<typename DataType = uint8_t, size_t Size = 4098>
class memory {
public:
	using data_t = DataType;
	static constexpr size_t size = Size;

	constexpr auto data() const { return _data.data(); }

	constexpr void clear() {
		_data.fill(0);
	}

	constexpr void write(data_t *data, size_t data_size, size_t position = 0) {
		const auto end = position + data_size;
		if (end > size) {
			// over.

		} else {
			memcpy(_data.data() + position, data, data_size);
		}
	}

	constexpr void write(data_t data, size_t position = 0) {
		if (position > size) {
			// over.

		} else {
			_data[position] = data;
		}
	}

	constexpr auto read(size_t index) const { return (index < size) ? _data[index] : 0; }
	constexpr void read(size_t index, data_t *data, size_t data_size) const {
		const auto end = index + data_size;
		if (end > size) {
			memcpy(data, _data.data(), data_size - (end - size));

		} else {
			memcpy(data, _data.data(), data_size);
		}
	}

private:
	std::array<data_t, size> _data;
};

class cpu {
public:
	using ram_t = memory<>;
	using vram_t = memory<uint8_t, 64 * 32>;

	using register_t = uint8_t;
	static constexpr size_t num_registers = 16;

	using index_register_t = uint16_t;

	using stack_t = uint16_t;
	using stack_index_t = size_t;
	static constexpr size_t max_stack = 16;

	using program_counter_t = uint16_t;
	static constexpr program_counter_t increment_pc = sizeof(program_counter_t);

	using timer_counter_t = uint16_t;

	using opcode_t = uint16_t;
	static constexpr size_t opcode_size = sizeof(opcode_t);

	constexpr static size_t program_address = 0x200;
	constexpr static size_t sprite_width = 8;
	constexpr static size_t sprite_height = 5;

	constexpr auto v(size_t index) const { return _registers[index]; }
	constexpr void v(size_t index, register_t value) { _registers[index] = value; }

	constexpr auto index_register() const { return _index_register; }
	constexpr void index_register(index_register_t i) { _index_register = i; }

	constexpr auto program_counter() const { return _program_counter; }
	constexpr void program_counter(program_counter_t pc) { _program_counter = pc; }
	constexpr void increment_program_counter(program_counter_t n = 0) { program_counter(program_counter() + increment_pc * n); }

	constexpr auto stack() const { return _stack[_stack_pointer]; }
	constexpr auto stack_pointer() const { return _stack_pointer; }
	constexpr void push_stack() {
		if (_stack_pointer < max_stack) {
			_stack[_stack_pointer] = program_counter();
			_stack_pointer++;
		}
	}
	constexpr void pop_stack() {
		if (_stack_pointer > 0) {
			_program_counter = stack();
			_stack_pointer--;
		}
	}

	constexpr auto delay_timer() const { return _delay_timer; }
	constexpr void delay_timer(timer_counter_t t) { _delay_timer = t; }

	constexpr auto sound_timer() const { return _sound_timer; }
	constexpr void sound_timer(timer_counter_t t) { _sound_timer = t; }
	constexpr bool sound() const { return sound_timer() > 0; }

	constexpr auto current_opcode() const { return _current_opcode; }

	constexpr const auto update_opcode() {
		_current_opcode = 0;

		for (size_t i = 0; i << opcode_size; ++i) {
			_current_opcode = _current_opcode << 8;
			_current_opcode |= _ram.read(program_counter() + i);
		}

		return _current_opcode;
	}

	void cycle() {
		update_opcode();
		dispatch();

		increment_program_counter();
		if (_delay_timer > 0) --_delay_timer;
		if (_sound_timer > 0) --_sound_timer;
	}

	void dispatch() {
		const auto opcode = current_opcode();
		const auto instruction = opcode & 0xF000;

		switch (instruction) {
		case 0x0000:
		{
			//op_0nnn();
			const auto sub_inst = opcode & 0x0FFF;
			switch (sub_inst) {
			case 0x00E0: op_00E0(); break;
			case 0x00EE: op_00EE(); break;
			default: op_error(); break;
			}
			break;
		}
		case 0x1000: op_1nnn(); break;
		case 0x2000: op_2nnn(); break;
		case 0x3000: op_3xkk(); break;
		case 0x4000: op_4xkk(); break;
		case 0x5000: op_5xy0(); break;
		case 0x6000: op_6xkk(); break;
		case 0x7000: op_7xkk(); break;
		case 0x8000:
		{
			const auto sub_inst = opcode & 0x000F;
			switch (sub_inst) {
			case 0x0000: op_8xy0(); break;
			case 0x0001: op_8xy1(); break;
			case 0x0002: op_8xy2(); break;
			case 0x0003: op_8xy3(); break;
			case 0x0004: op_8xy4(); break;
			case 0x0005: op_8xy5(); break;
			case 0x0006: op_8xy6(); break;
			case 0x0007: op_8xy7(); break;
			case 0x000E: op_8xyE(); break;
			default: op_error(); break;
			}
			break;
		}
		case 0x9000: op_9xy0(); break;
		case 0xA000: op_Annn(); break;
		case 0xB000: op_Bnnn(); break;
		case 0xC000: op_Cxkk(); break;
		case 0xD000: op_Dxyn(); break;
		case 0xE000: 
		{
			const auto sub_inst = opcode & 0x00FF;
			switch (sub_inst) {
			case 0x009E: op_Ex9E(); break;
			case 0x00A1: op_ExA1(); break;
			default: op_error(); break;
			}
			break;
		}
		case 0xF000:
		{
			const auto sub_inst = opcode & 0x00FF;
			switch (sub_inst) {
			case 0x0007: op_Fx07(); break;
			case 0x000A: op_Fx0A(); break;
			case 0x0015: op_Fx15(); break;
			case 0x0018: op_Fx18(); break;
			case 0x001E: op_Fx1E(); break;
			case 0x0029: op_Fx29(); break;
			case 0x0033: op_Fx33(); break;
			case 0x0055: op_Fx55(); break;
			case 0x0065: op_Fx65(); break;
			default: op_error(); break;
			}
			break;
		}
		}
	}

	// SYS addr
	void op_0nnn() {
		// Jump to a machine code routine at nnn.
		const auto addr = static_cast<program_counter_t>(current_opcode() & 0x0FFFU);
		program_counter(addr);
	}

	// CLS
	void op_00E0() {
		// Clear the display.
		_vram.clear();
	}

	// RET
	void op_00EE() {
		// Return from a subroutine.
		pop_stack();
	}

	// JP addr
	void op_1nnn() {
		// Jump to location nnn.
		const auto nnn = static_cast<program_counter_t>(current_opcode() & 0x0FFFU);
		program_counter(nnn);
	}

	// CALL addr
	void op_2nnn() {
		// Call subroutine at nnn.
		const auto nnn = static_cast<program_counter_t>(current_opcode() & 0x0FFFU);
		push_stack();
		program_counter(nnn);
	}

	// SE Vx, byte
	void op_3xkk() {
		// Skip next instruction if Vx = kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		if (v(x) == kk) {
			increment_program_counter();
		}
	}

	// SNE Vx, byte
	void op_4xkk() {
		// Skip next instruction if Vx != kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		if (v(x) != kk) {
			increment_program_counter();
		}
	}

	// SE Vx, Vy
	void op_5xy0() {
		// Skip next instruction if Vx = Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		if (v(x) == v(y)) {
			increment_program_counter();
		}
	}

	// LD Vx, byte
	void op_6xkk() {
		// Set Vx = kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		v(x, kk);
	}

	// ADD Vx, byte
	void op_7xkk() {
		// Set Vx = Vx + kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		v(x, v(x) + kk);
	}

	// LD Vx, Vy
	void op_8xy0() {
		// Set Vx = Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		v(x, v(y));
	}

	// OR Vx, Vy
	void op_8xy1() {
		// Set Vx = Vx OR Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		v(x, v(x) | v(y));
	}

	// AND Vx, Vy
	void op_8xy2() {
		// Set Vx = Vx AND Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		v(x, v(x) & v(y));
	}

	// XOR Vx, Vy
	void op_8xy3() {
		// Set Vx = Vx XOR Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		v(x, v(x) ^ v(y));
	}

	// ADD Vx, Vy
	void op_8xy4() {
		// Set Vx = Vx + Vy, set VF = carry.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		const auto sum = static_cast<long long>(v(x)) + static_cast<long long>(v(y));
		v(x, static_cast<register_t>(sum & 0xFF));
		v(0xF, (sum > 0xFF) ? 1 : 0);
	}

	// SUB Vx, Vy
	void op_8xy5() {
		// Set Vx = Vx - Vy, set VF = NOT borrow.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		const auto sub = static_cast<long long>(v(x)) - static_cast<long long>(v(y));
		v(0xF, (v(x) > v(y)) ? 1 : 0);
		v(x, static_cast<register_t>(sub & 0xFF));
	}

	// SHR Vx {, Vy}
	void op_8xy6() {
		// Set Vx = Vx SHR 1.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		v(0xF, (v(x) & 0x01) ? 1 : 0);
		v(x, v(x) >> 1);
	}

	// SUBN Vx, Vy
	void op_8xy7() {
		// Set Vx = Vy - Vx, set VF = NOT borrow.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		const auto sub = static_cast<long long>(v(y)) - static_cast<long long>(v(x));
		v(0xF, (v(y) > v(x)) ? 1 : 0);
		v(x, static_cast<register_t>(sub & 0xFF));
	}

	// SHL Vx {, Vy}
	void op_8xyE() {
		// Set Vx = Vx SHL 1.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		v(0xF, (v(x) & 0x80) ? 1 : 0);
		v(x, v(x) << 1);
	}

	// SNE Vx, Vy
	void op_9xy0() {
		// Skip next instruction if Vx != Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		if (v(x) != v(y)) {
			increment_program_counter();
		}
	}

	// LD I, addr
	void op_Annn() {
		// Set I = nnn.
		const auto nnn = static_cast<program_counter_t>(current_opcode() & 0x0FFFU);
		index_register(nnn);
	}

	// JP V0, addr
	void op_Bnnn() {
		// Jump to location nnn + V0.
		const auto nnn = static_cast<program_counter_t>(current_opcode() & 0x0FFFU);
		program_counter(nnn + v(0));
	}

	// RND Vx, byte
	void op_Cxkk() {
		// Set Vx = random byte AND kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		const auto rnd = 0; // TODO
		v(x, rnd & kk);
	}

	// DRW Vx, Vy, nibble
	void op_Dxyn() {
		// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U);
		const auto n = static_cast<size_t>(current_opcode() & 0x000FU);
		std::vector<vram_t::data_t> reading_data;
		reading_data.resize(n);
		_ram.read(index_register(), reading_data.data(), reading_data.size());
		// TODO
	}

	// SKP Vx
	void op_Ex9E() {
		// Skip next instruction if key with the value of Vx is pressed.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		if (v(x) & 0) {
			increment_program_counter();
		}
		// TODO
	}

	// SKNP Vx
	void op_ExA1() {
		// Skip next instruction if key with the value of Vx is not pressed.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		if (v(x) & 0) {
			increment_program_counter();
		}
		// TODO
	}

	// LD Vx, DT
	void op_Fx07() {
		// Set Vx = delay timer value.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		v(x, static_cast<register_t>(delay_timer()));
	}

	// LD Vx, K
	void op_Fx0A() {
		// Wait for a key press, store the value of the key in Vx.
		// TODO
	}

	// LD DT, Vx
	void op_Fx15() {
		// Set delay timer = Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		delay_timer(v(x));
	}

	// LD ST, Vx
	void op_Fx18() {
		// Set sound timer = Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		sound_timer(v(x));
	}

	// ADD I, Vx
	void op_Fx1E() {
		// Set I = I + Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		index_register(index_register() + v(x));
	}

	// LD F, Vx
	void op_Fx29() {
		// Set I = location of sprite for digit Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		index_register(v(x) * sprite_height);
	}

	// LD B, Vx
	void op_Fx33() {
		// Store BCD representation of Vx in memory locations I, I+1, and I+2.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		const auto b = v(x) / 100 % 10;
		const auto c = v(x) / 10 % 10;
		const auto d = v(x) % 10;
		_ram.write(index_register(), b);
		_ram.write(index_register() + 1, c);
		_ram.write(index_register() + 2, d);
	}

	// LD [I], Vx
	void op_Fx55() {
		// Store registers V0 through Vx in memory starting at location I.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		for (size_t i = 0; i < x; ++i) {
			_ram.write(index_register() + i, v(i));
		}
	}

	// LD Vx, [I]
	void op_Fx65() {
		// Read registers V0 through Vx from memory starting at location I.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U);
		for (size_t i = 0; i < x; ++i) {
			v(i, _ram.read(index_register()));
		}
	}

	void op_error() {

	}

private:
	ram_t _ram;
	vram_t _vram;

	std::array<register_t, num_registers> _registers;
	index_register_t _index_register;

	std::array<stack_t, max_stack> _stack;
	stack_index_t _stack_pointer;

	program_counter_t _program_counter;
	opcode_t _current_opcode;

	timer_counter_t _delay_timer;
	timer_counter_t _sound_timer;
};

template<unsigned Width = 64, unsigned Height = 32, typename PixelType = unsigned int>
class video {
public:
	static constexpr unsigned width = Width;
	static constexpr unsigned height = Height;
	static constexpr size_t size = width * height;
	static constexpr auto aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

	using pixel_t = PixelType;
	using coord_t = int;
	using framebuffer_t = std::array<pixel_t, size>;

	constexpr void set(coord_t x, coord_t y, pixel_t pixel = 0) {
		const size_t index = static_cast<size_t>(y) * width + static_cast<size_t>(x);
		if (index < _framebuffer.size()) {
			_framebuffer[index] = pixel;
		}
	}

	constexpr pixel_t* framebuffer() { return _framebuffer.data(); }

private:
	framebuffer_t _framebuffer{};
};

} // namespace chip8

