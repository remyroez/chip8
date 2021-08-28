#pragma once

#include <array>
#include <limits>

namespace chip8 {

enum class key : size_t {
	key_1,
	key_2,
	key_3,
	key_4,
	key_5,
	key_6,
	key_7,
	key_8,
	key_9,
	key_0,
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

	constexpr auto stack_pointer() const { return _stack_pointer; }
	constexpr auto program_counter() const { return _program_counter; }
	constexpr auto current_opcode() const { return _current_opcode; }
	constexpr auto delay_timer() const { return _delay_timer; }
	constexpr auto sound_timer() const { return _sound_timer; }
	constexpr bool sound() const { return sound_timer() > 0; }

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

		_program_counter += increment_pc;
		if (_delay_timer > 0) --_delay_timer;
		if (_sound_timer > 0) --_sound_timer;
	}

	void dispatch() {
		const auto opcode = current_opcode();
		const auto instruction = opcode & 0xF000;

		switch (instruction) {
		case 0x0000: op_0nnn(); break;
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

	void op_0nnn() {

	}

	void op_1nnn() {

	}

	void op_2nnn() {

	}

	void op_3xkk() {

	}

	void op_4xkk() {

	}

	void op_5xy0() {

	}

	void op_6xkk() {

	}

	void op_7xkk() {

	}

	void op_8xy0() {

	}

	void op_8xy1() {

	}

	void op_8xy2() {

	}

	void op_8xy3() {

	}

	void op_8xy4() {

	}

	void op_8xy5() {

	}

	void op_8xy6() {

	}

	void op_8xy7() {

	}

	void op_8xyE() {

	}

	void op_9xy0() {

	}

	void op_Annn() {

	}

	void op_Bnnn() {

	}

	void op_Cxkk() {

	}

	void op_Dxyn() {

	}

	void op_Ex9E() {

	}

	void op_ExA1() {

	}

	void op_Fx07() {

	}

	void op_Fx0A() {

	}

	void op_Fx15() {

	}

	void op_Fx18() {

	}

	void op_Fx1E() {

	}

	void op_Fx29() {

	}

	void op_Fx33() {

	}

	void op_Fx55() {

	}

	void op_Fx65() {

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

