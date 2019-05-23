#ifndef __SPROTO_BIND_H__
#define __SPROTO_BIND_H__

extern "C" {
	struct sproto;
	struct sproto_type;
};

#include <Godot.hpp>
#include <Reference.hpp>
#include <Array.hpp>
#include <Dictionary.hpp>
#include <PoolArrays.hpp>
#include <Variant.hpp>

namespace godot {

	class Sproto : public Reference {
		GODOT_CLASS(Sproto, Reference)
	private:
		struct sproto * m_sproto;
		PoolByteArray m_packBuffer;
		PoolByteArray m_unpackBuffer;

		PoolByteArray m_encodeBuffer;
		PoolByteArray m_decodeBuffer;
	protected:
		struct sproto_type * get_sproto_type(const String& name);
	public:
		void _init();
		static void _register_methods();
	public:
		Sproto();
		~Sproto();

		void newsproto(const Array buffer);
		void dumpsproto();
		Variant encode(const String name, const Dictionary data);
		Variant decode(const String name, const Array buffer);
		Variant pack(const Array buffer);
		Variant unpack(const Array buffer);

		Variant protocol(const Variant name);

		Variant test(const String name, const Dictionary data);
	};

}

#endif