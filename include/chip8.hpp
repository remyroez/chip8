#pragma once

#include <array>
#include <vector>
#include <limits>
#include <random>

namespace chip8 {

template<typename DataType = uint8_t, size_t Size = 4098>
class memory {
public:
	using data_t = DataType;
	static constexpr size_t size = Size;
	static constexpr data_t data_max = std::numeric_limits<data_t>::max();
	static constexpr data_t data_bits = std::numeric_limits<data_t>::digits;

	constexpr auto data() const { return _data.data(); }

	constexpr void clear() {
		_data.fill(0);
	}

	constexpr void write(const data_t *data, size_t data_size, size_t position = 0) {
		const auto end = position + data_size;
		if (end > size) {
			// over.

		} else {
			memcpy(_data.data() + position, data, data_size);
		}
	}

	constexpr void write(const data_t &data, size_t position = 0) {
		if (position > size) {
			// over.

		} else {
			_data[position] = data;
		}
	}

	constexpr void write_bit(const data_t &data, size_t position, size_t offset = 0) {
		if (offset == 0) {
			write(data, position);

		} else {
			const auto front_mask = (data_max >> offset) & data_max;
			const auto back_mask = (data_max << (data_bits - offset)) & data_max;
			const auto data_front = data >> offset & data_max;
			const auto data_back = data << offset & data_max;
			if (position > size) {
				// over.

			} else {
				const auto before_front = read(position) & ~front_mask;
				write(before_front | data_front, position);

				const auto before_back = read(position + 1) & ~back_mask;
				write(before_back | data_back, position + 1);
			}
		}
	}

	constexpr auto read(size_t index) const { return (index < size) ? _data[index] : _data[size - 1]; }
	constexpr void read(size_t index, data_t *data, size_t data_size) const {
		const auto end = index + data_size;
		if (end > size) {
			memcpy(data, _data.data() + index, data_size - (end - size));

		} else {
			memcpy(data, _data.data() + index, data_size);
		}
	}

private:
	std::array<data_t, size> _data;
};

class cpu {
public:
	using ram_t = memory<>;

	static constexpr size_t video_width = 64;
	static constexpr size_t video_height = 32;
	using video_t = uint8_t;
	static constexpr size_t video_bit_size = std::numeric_limits<video_t>::digits;
	using vram_t = memory<video_t, video_width / video_bit_size * video_height>;

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
	constexpr static size_t default_sprite_address = 0x50;
	constexpr static size_t sprite_width = 8;
	constexpr static size_t sprite_height = 5;

	constexpr static size_t num_key = static_cast<size_t>(16);
	using input_t = uint8_t;

public:
	cpu() :
		_ram{},
		_vram{},
		_registers{},
		_index_register(0),
		_stack{},
		_stack_pointer(0),
		_program_counter(program_address),
		_current_opcode(0),
		_delay_timer(0),
		_sound_timer(0),
		_inputs{},
		_random_device{},
		_random_engine(_random_device()),
		_rand(0, UINT8_MAX) {}

public:
	constexpr auto v(size_t index) const { return _registers[index]; }
	constexpr void v(size_t index, register_t value) { _registers[index] = value; }

	constexpr auto index_register() const { return _index_register; }
	constexpr void index_register(index_register_t i) { _index_register = i; }

	constexpr auto program_counter() const { return _program_counter; }
	constexpr void program_counter(program_counter_t pc) { _program_counter = pc; }
	constexpr void increment_program_counter(program_counter_t n = 1) { program_counter(program_counter() + increment_pc * n); }

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

	constexpr auto input(size_t k) { return _inputs[k]; }
	constexpr auto input(size_t k, input_t s) { _inputs[k] = s; }

	auto random() { return _rand(_random_engine); }

	constexpr auto current_opcode() const { return _current_opcode; }

	constexpr const auto update_opcode() {
		_current_opcode = 0;

		for (size_t i = 0; i < opcode_size; ++i) {
			_current_opcode = _current_opcode << 8;
			_current_opcode |= _ram.read(program_counter() + i);
		}

		return _current_opcode;
	}

	void cycle() {
		if (program_counter() < ram_t::size) {
			update_opcode();
			dispatch();
			increment_program_counter();
		}

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
		const auto nnn = static_cast<program_counter_t>(current_opcode() & 0x0FFFU);
		program_counter(nnn);
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
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		if (v(x) == kk) {
			increment_program_counter();
		}
	}

	// SNE Vx, byte
	void op_4xkk() {
		// Skip next instruction if Vx != kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		if (v(x) != kk) {
			increment_program_counter();
		}
	}

	// SE Vx, Vy
	void op_5xy0() {
		// Skip next instruction if Vx = Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		if (v(x) == v(y)) {
			increment_program_counter();
		}
	}

	// LD Vx, byte
	void op_6xkk() {
		// Set Vx = kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		v(x, kk);
	}

	// ADD Vx, byte
	void op_7xkk() {
		// Set Vx = Vx + kk.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		v(x, v(x) + kk);
	}

	// LD Vx, Vy
	void op_8xy0() {
		// Set Vx = Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		v(x, v(y));
	}

	// OR Vx, Vy
	void op_8xy1() {
		// Set Vx = Vx OR Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		v(x, v(x) | v(y));
	}

	// AND Vx, Vy
	void op_8xy2() {
		// Set Vx = Vx AND Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		v(x, v(x) & v(y));
	}

	// XOR Vx, Vy
	void op_8xy3() {
		// Set Vx = Vx XOR Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		v(x, v(x) ^ v(y));
	}

	// ADD Vx, Vy
	void op_8xy4() {
		// Set Vx = Vx + Vy, set VF = carry.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		const auto sum = static_cast<long long>(v(x)) + static_cast<long long>(v(y));
		v(x, static_cast<register_t>(sum & 0xFF));
		v(0xF, (sum > 0xFF) ? 1 : 0);
	}

	// SUB Vx, Vy
	void op_8xy5() {
		// Set Vx = Vx - Vy, set VF = NOT borrow.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		const auto sub = static_cast<long long>(v(x)) - static_cast<long long>(v(y));
		v(0xF, (v(x) > v(y)) ? 1 : 0);
		v(x, static_cast<register_t>(sub & 0xFF));
	}

	// SHR Vx {, Vy}
	void op_8xy6() {
		// Set Vx = Vx SHR 1.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		v(0xF, (v(x) & 0x01) ? 1 : 0);
		v(x, v(x) >> 1);
	}

	// SUBN Vx, Vy
	void op_8xy7() {
		// Set Vx = Vy - Vx, set VF = NOT borrow.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		const auto sub = static_cast<long long>(v(y)) - static_cast<long long>(v(x));
		v(0xF, (v(y) > v(x)) ? 1 : 0);
		v(x, static_cast<register_t>(sub & 0xFF));
	}

	// SHL Vx {, Vy}
	void op_8xyE() {
		// Set Vx = Vx SHL 1.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		v(0xF, (v(x) & 0x80) ? 1 : 0);
		v(x, v(x) << 1);
	}

	// SNE Vx, Vy
	void op_9xy0() {
		// Skip next instruction if Vx != Vy.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
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
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto kk = static_cast<register_t>(current_opcode() & 0x00FFU);
		const auto rnd = random();
		v(x, rnd & kk);
	}

	// DRW Vx, Vy, nibble
	void op_Dxyn() {
		// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		const auto y = static_cast<size_t>(current_opcode() & 0x00F0U) >> 4;
		const auto n = static_cast<size_t>(current_opcode() & 0x000FU);

		std::vector<vram_t::data_t> sprite;
		sprite.resize(n);
		_ram.read(index_register(), sprite.data(), sprite.size());
		
		const auto loc_x = v(x);
		const auto ofs = loc_x % video_bit_size;
		const auto loc_y = v(y);

		v(0xF, 0);
		for (size_t coord_y = 0; coord_y < n; ++coord_y) {
			const auto pixel = sprite[coord_y];
			const auto pos_y = (loc_y + coord_y) * (video_width / video_bit_size);
			for (size_t coord_x = 0; coord_x < sprite_width; ++coord_x) {
				const auto pos_x = (loc_x + ((ofs > 0 ? 1 : 0) - (coord_x + ofs) / video_bit_size)) / video_bit_size;
				const auto pos = pos_x + pos_y;
				const auto index = (coord_x + ofs) % video_bit_size;
				auto chunk = _vram.read(pos);
				const auto pt = 1 << index;
				const auto dot = pixel & pt;
				if ((chunk & dot) != 0) {
					v(0xF, 1);
				}
				chunk ^= dot;
				_vram.write(chunk, pos);
			}
		}
	}

	// SKP Vx
	void op_Ex9E() {
		// Skip next instruction if key with the value of Vx is pressed.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		if (input(v(x)) != 0) {
			increment_program_counter();
		}
	}

	// SKNP Vx
	void op_ExA1() {
		// Skip next instruction if key with the value of Vx is not pressed.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		if (input(v(x)) == 0) {
			increment_program_counter();
		}
	}

	// LD Vx, DT
	void op_Fx07() {
		// Set Vx = delay timer value.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		v(x, static_cast<register_t>(delay_timer()));
	}

	// LD Vx, K
	void op_Fx0A() {
		// Wait for a key press, store the value of the key in Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		for (size_t i = 0; i < num_key; ++i) {
			if (input(i)) {
				v(x, i);
				return;
			}
		}
		program_counter(program_counter() - increment_pc);
	}

	// LD DT, Vx
	void op_Fx15() {
		// Set delay timer = Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		delay_timer(v(x));
	}

	// LD ST, Vx
	void op_Fx18() {
		// Set sound timer = Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		sound_timer(v(x));
	}

	// ADD I, Vx
	void op_Fx1E() {
		// Set I = I + Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		index_register(index_register() + v(x));
	}

	// LD F, Vx
	void op_Fx29() {
		// Set I = location of sprite for digit Vx.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		index_register(v(x) * sprite_height);
	}

	// LD B, Vx
	void op_Fx33() {
		// Store BCD representation of Vx in memory locations I, I+1, and I+2.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
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
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		for (size_t i = 0; i < x; ++i) {
			_ram.write(index_register() + i, v(i));
		}
	}

	// LD Vx, [I]
	void op_Fx65() {
		// Read registers V0 through Vx from memory starting at location I.
		const auto x = static_cast<size_t>(current_opcode() & 0x0F00U) >> 8;
		for (size_t i = 0; i < x; ++i) {
			v(i, _ram.read(index_register()));
		}
	}

	void op_error() {

	}

	void clear_ram() { _ram.clear(); }

	void clear_vram() { _vram.clear(); }

	void load_default_sprites() {
		const ram_t::data_t dataset[] = {
			0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
			0x20, 0x60, 0x20, 0x20, 0x70, // 1
			0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
			0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
			0x90, 0x90, 0xF0, 0x10, 0x10, // 4
			0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
			0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
			0xF0, 0x10, 0x20, 0x40, 0x40, // 7
			0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
			0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
			0xF0, 0x90, 0xF0, 0x90, 0x90, // A
			0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
			0xF0, 0x80, 0x80, 0x80, 0xF0, // C
			0xE0, 0x90, 0x90, 0x90, 0xE0, // D
			0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
			0xF0, 0x80, 0xF0, 0x80, 0x80  // F
		};
		_ram.write(dataset, sizeof(dataset), default_sprite_address);
	}

	void load_rom(const ram_t::data_t *data, size_t data_size) {
		_ram.write(data, data_size, program_address);
	}

	void boot() {
		reset();
		load_default_sprites();
	}

	void reset() {
		clear_ram();
		clear_vram();
		_registers = {};
		_index_register = 0;
		_stack = {};
		_stack_pointer = 0;
		_program_counter = program_address;
		_current_opcode = 0;
		_delay_timer = 0;
		_sound_timer = 0;
		_inputs = {};
		//_random_device;
		//_random_engine;
		_rand.reset();
	}

	constexpr auto vram_data() const { return _vram.data(); }

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

	std::array<input_t, num_key> _inputs;

	std::random_device _random_device;
	std::default_random_engine _random_engine;
	std::uniform_int_distribution<> _rand;
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

