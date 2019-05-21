#include "sproto_bind.hpp"

extern "C" {
#include "sproto.h"
};

#define ENCODE_DEEPLEVEL 64
#define ENCODE_MAXSIZE 0x1000000
#define ENCODE_BUFFERSIZE 2050

using namespace godot;

Sproto::Sproto()
{
	m_packBuffer.resize(ENCODE_BUFFERSIZE);
	m_unpackBuffer.resize(ENCODE_BUFFERSIZE);
	m_encodeBuffer.resize(ENCODE_BUFFERSIZE);
	m_decodeBuffer.resize(ENCODE_BUFFERSIZE);
}

Sproto::~Sproto()
{
	if (m_sproto != NULL) {
		sproto_release(m_sproto);
		m_sproto = NULL;
	}
}

void godot::Sproto::newsproto(const PoolByteArray buffer)
{
	PoolByteArray::Read r = buffer.read();
	const uint8_t * ptr = r.ptr();
	int sz = buffer.size();
	struct sproto * sp = sproto_create(buffer.read().ptr(), buffer.size());
	if (sp == NULL) {
		Godot::print_error("create sproto error", "newsproto", __FILE__, __LINE__);
		return;
	}
	m_sproto = sp;
	return;
}

void godot::Sproto::dumpsproto()
{
	if (m_sproto == NULL) {
		Godot::print_error("sproto is NULL", "dumpsproto", __FILE__, __LINE__);
		return;
	}
	sproto_dump(m_sproto);
}

struct sproto_type * godot::Sproto::get_sproto_type(const String & name)
{
	if (m_sproto == NULL) {
		Godot::print_error("sproto is NULL", "get_sproto_type", __FILE__, __LINE__);
		return NULL;
	}
	return sproto_type(m_sproto, name.utf8().get_data());;
}

struct encode_ud {
	struct sproto_type * st;
	Variant tbl;
	const char * array_tag;
	int array_index;
	int deep;
};

static int encode_callback(const struct sproto_arg * args) {
	struct encode_ud * self = (struct encode_ud *) args->ud;
	Variant tbl = self->tbl;
	if (self->deep >= ENCODE_DEEPLEVEL) {
		Godot::print_error("The table is too deep", "encode_callback", __FILE__, __LINE__);
		return SPROTO_CB_ERROR;
	}

	Variant target;
	Variant& tagValue = tbl.operator Dictionary()[args->tagname];
	if (args->index > 0) {
		if (args->tagname != self->array_tag) {
			self->array_tag = args->tagname;
			Variant::Type tagType = tagValue.get_type();
			if (tagType != Variant::ARRAY && tagType != Variant::DICTIONARY) {
				self->array_index = 0;
				return SPROTO_CB_NIL;
			}
			if ((tagType == Variant::ARRAY && tagValue.operator Array().size() == 0) || (tagType == Variant::DICTIONARY && tagValue.operator Dictionary().size() == 0)) {
				self->array_index = 0;
				return SPROTO_CB_NOARRAY;
			}
			if (tagType == Variant::DICTIONARY) {
				tbl.operator Dictionary()[args->tagname] = tagValue.operator Dictionary().values();
			}
			target = tagValue.operator godot::Array()[args->index - 1];
			if (target.get_type() == Variant::NIL) {
				return SPROTO_CB_NIL;
			}
		}
		else {
			if (tagValue.operator godot::Array().size() < args->index) {
				return SPROTO_CB_NIL;
			}
			else {
				target = tagValue.operator godot::Array()[args->index - 1];
			}
		}
	}
	else {
		target = tagValue;
	}
	
	if (target.get_type() == Variant::NIL) {
		return SPROTO_CB_NIL;
	}

	switch (args->type)
	{
	case SPROTO_TINTEGER: {
		if (target.get_type() != Variant::INT && target.get_type() != Variant::REAL) {
			Godot::print_error(String(args->tagname) + "(" +String::num(args->tagid) +")should be a int/real.", "encode_callback", __FILE__, __LINE__);
			return SPROTO_CB_ERROR;
		}
		int64_t v;
		if (args->extra) {
			if (target.get_type() == Variant::INT) {
				v = (int64_t)(target.operator signed int() * args->extra);
			}
			else {
				v = (int64_t)(round(target.operator double() * args->extra));
			}
		}
		else {
			if (target.get_type() == Variant::INT) {
				v = (int64_t)(target.operator signed int());
			}
			else {
				v = (int64_t)(round(target.operator double()));
			}
		}
		int64_t vh = v >> 31;
		if (vh == 0 || vh == -1) {
			*(uint32_t *)args->value = (uint32_t)v;
			return 4;
		}
		else {
			*(uint64_t *)args->value = (uint64_t)v;
			return 8;
		}
	}
	case SPROTO_TBOOLEAN: {
		if (target.get_type() != Variant::BOOL) {
			Godot::print_error(String(args->tagname) + "(" + String::num(args->tagid) + ")should be a Bool.", "encode_callback", __FILE__, __LINE__);
			return SPROTO_CB_ERROR;
		}
		*(int *)args->value = target.operator bool();
		return 4;
	}
	case SPROTO_TSTRING: {
		if (target.get_type() != Variant::STRING && target.get_type() != Variant::POOL_BYTE_ARRAY) {
			Godot::print_error(String(args->tagname) + "(" + String::num(args->tagid) + ")should be a String/PoolByteArray.", "encode_callback", __FILE__, __LINE__);
			return SPROTO_CB_ERROR;
		}
		int sz = 0;
		if (args->extra != 0) {
			sz = target.operator godot::PoolByteArray().size();
			if (sz > args->length)
				return SPROTO_CB_ERROR;
			memcpy(args->value, target.operator godot::PoolByteArray().read().ptr(), sz);
		}
		else {
			sz = target.operator godot::String().utf8().length();
			if (sz > args->length)
				return SPROTO_CB_ERROR;
			memcpy(args->value, target.operator godot::String().utf8().get_data(), sz);
		}
		
		return sz;
	}
	case SPROTO_TSTRUCT: {
		struct encode_ud sub;
		memset(&sub, 0x00, sizeof(sub));
		sub.st = args->subtype;
		sub.deep = self->deep + 1;
		sub.tbl = target;
		int r = sproto_encode(args->subtype, args->value, args->length, encode_callback, &sub);
		if (r < 0) {
			return SPROTO_CB_ERROR;
		}
		return r;
	}
	default:
		Godot::print_error("Invalid field type " + String::num(args->type), "encode_callback", __FILE__, __LINE__);
		break;
	}

	return SPROTO_CB_ERROR;
}

Variant godot::Sproto::encode(const String name, const Dictionary data)
{
	Variant result;
	struct sproto_type * st = get_sproto_type(name);
	if (st == NULL) {
		Godot::print_error("this sproto type is NULL", "encode", __FILE__, __LINE__);
		return result;
	}
	
	struct encode_ud self;
	memset(&self, 0x00, sizeof(struct encode_ud));
	self.tbl = Variant(data);

	
	for (;;) {
		int r = 0;
		self.array_tag = NULL;
		self.array_index = 0;
		self.deep = 0;
		
		{
			PoolByteArray::Write w = m_encodeBuffer.write();
			r = sproto_encode(st, w.ptr(), m_encodeBuffer.size(), encode_callback, &self);
		}
		
		if (r < 0) {
			m_encodeBuffer.resize(m_encodeBuffer.size() * 2);
		}
		else {
			PoolByteArray tmp(m_encodeBuffer);
			tmp.resize(r);
			result = tmp;
			return result;
		}
	}

	return result;
}


struct decode_ud {
	Variant result;
	const char * array_tag;
	int deep;
	int array_index;
	int result_index;
	int mainindex_tag;
	int key_index;
};

static int decode_callback(const struct sproto_arg *args) {
	struct decode_ud * self = (struct decode_ud *)args->ud;
	Variant& result = self->result;
	Variant value;
	if (self->deep >= ENCODE_DEEPLEVEL) {
		Godot::print_error("The table is too deep", "decode_callback", __FILE__, __LINE__);
		return SPROTO_CB_ERROR;
	}
	if (args->index != 0) {
		if (args->tagname != self->array_tag) {
			self->array_tag = args->tagname;
			if (args->mainindex >= 0) {
				result.operator godot::Dictionary()[args->tagname] = Dictionary();
			}
			else {
				result.operator godot::Dictionary()[args->tagname] = Array();
			}
			if (args->index < 0) {
				return 0;
			}
		}
	}

	switch (args->type)
	{
	case SPROTO_TINTEGER: {
		if (args->extra != 0) {
			int64_t v = *(int64_t *)args->value;
			value = (double)v / (double)args->extra;
		}
		else {
			int64_t v = *(int64_t *)args->value;
			value = v;
		}
		break;
	}
	case SPROTO_TBOOLEAN: {
		value = (bool)(*(uint64_t *)args->value);
		break;
	}
	case SPROTO_TSTRING: {
		if (args->extra != 0) {
			PoolByteArray strBuf;
			strBuf.resize(args->length);
			PoolByteArray::Write w = strBuf.write();
			memcpy(w.ptr(), args->value, args->length);
			value = strBuf;
		}
		else {
			char * tmp = (char *)malloc(args->length+1);
			memset(tmp, 0x00, args->length + 1);
			memcpy(tmp, args->value, args->length);
			String str(tmp);
			int len = str.length();
			value = str;
			free(tmp);
			tmp = NULL;
		}
		break;
	}
	case SPROTO_TSTRUCT: {
		struct decode_ud sub;
		int r;
		sub.deep = self->deep + 1;
		sub.array_index = 0;
		sub.array_tag = NULL;
		sub.result = Dictionary();
		if (args->mainindex >= 0) {
			sub.mainindex_tag = args->mainindex;
			r = sproto_decode(args->subtype, args->value, args->length, decode_callback, &sub);
			if (r < 0 || r != args->length) {
				return r;
			}
			value = sub.result;
		}
		else {
			sub.mainindex_tag = -1;
			sub.key_index = 0;
			r = sproto_decode(args->subtype, args->value, args->length, decode_callback, &sub);
			if (r < 0) {
				return SPROTO_CB_ERROR;
			}
			if (r != args->length) {
				return r;
			}
			value = sub.result;
		}
		break;
	}
	default: {
		Godot::print_error("Invalid type", "decode_callback", __FILE__, __LINE__);
		break;
	}
	}
	//Godot::print("this data of " + String(args->tagname) + " is " + value.operator godot::String());
	if (args->index > 0) {
		if (args->mainindex >= 0) {
			String _mainindex = value.operator godot::Dictionary()["_mainindex"];
			value.operator godot::Dictionary().erase("_mainindex");
			result.operator godot::Dictionary()[args->tagname].operator godot::Dictionary()[_mainindex] = value;
		}
		else {
			result.operator godot::Dictionary()[args->tagname].operator godot::Array().push_back(value);
		}
	}
	else {
		if (self->mainindex_tag >= 0 && self->mainindex_tag == args->tagid) {
			result.operator godot::Dictionary()["_mainindex"] = value;
		}
		result.operator godot::Dictionary()[args->tagname] = value;
	}

	return 0;
}

Variant godot::Sproto::decode(const String name, const PoolByteArray buffer)
{
	Variant result;
	struct sproto_type * st = get_sproto_type(name);
	if (st == NULL) {
		Godot::print_error("this sproto type is NULL", "decode", __FILE__, __LINE__);
		return result;
	}
	/*
	struct decode_ud self;
	self.result = Dictionary();
	self.deep = 0;
	self.array_index = 0;
	self.array_tag = NULL;
	self.mainindex_tag = -1;
	self.key_index = 0;
	PoolByteArray::Read r = buffer.read();
	int ret = sproto_decode(st, r.ptr(), buffer.size(), decode_callback, &self);
	if (ret < 0) {
		Godot::print_error("decode error", "decode", __FILE__, __LINE__);
		return result;
	}
	result = self.result;
	*/
	return result;
}

static void expand_buffer(PoolByteArray & buffer, int nsz) {
	PoolByteArray::Read r = buffer.read();
	if (nsz > ENCODE_MAXSIZE) {
		Godot::print_error("expand_buffer err, object is too large, max size limit " + String::num(ENCODE_MAXSIZE), "expand_buffer", __FILE__, __LINE__);
		return;
	}
	buffer.resize(nsz);
}

Variant godot::Sproto::pack(const PoolByteArray buffer)
{
	Variant result;
	int bytes;
	int sz = buffer.size();
	int maxsz = (sz + 2047) / 2048 * 2 + sz;
	if (m_packBuffer.size() < maxsz) {
		expand_buffer(m_packBuffer, maxsz);
	}
	{
		PoolByteArray::Write w = m_packBuffer.write();
		bytes = sproto_pack(buffer.read().ptr(), sz, w.ptr(), maxsz);
	}
	if (bytes > maxsz) {
		Godot::print_error("pack err, return size = " + String::num(bytes) + ", max size limit " + String::num(ENCODE_MAXSIZE), "pack", __FILE__, __LINE__);
		return result;
	}
	PoolByteArray tmpBuffer = PoolByteArray();
	tmpBuffer.resize(bytes);
	PoolByteArray::Write w = tmpBuffer.write();
	memcpy(w.ptr(), m_packBuffer.read().ptr(), bytes);
	result = tmpBuffer;

	return result;
}

Variant godot::Sproto::unpack(const PoolByteArray buffer)
{
	Variant result;
	int ret = 0;
	int sz = buffer.size();
	int osz = m_unpackBuffer.size();
	{
		PoolByteArray::Read r = buffer.read();
		PoolByteArray::Write w = m_unpackBuffer.write();
		ret = sproto_unpack(r.ptr(), sz, w.ptr(), osz);
	}
	if (ret < 0) {
		Godot::print_error("Invalid unpack stream", "unpack", __FILE__, __LINE__);
		return result;
	}
	if (ret > osz) {
		expand_buffer(m_unpackBuffer, ret);
		PoolByteArray::Read r = buffer.read();
		PoolByteArray::Write w = m_unpackBuffer.write();
		ret = sproto_unpack(r.ptr(), sz, w.ptr(), ret);
	}
	if (ret < 0) {
		Godot::print_error("Invalid unpack stream", "unpack", __FILE__, __LINE__);
		return result;
	}
	PoolByteArray tmpBuf = PoolByteArray();
	tmpBuf.resize(ret);
	memcpy(tmpBuf.write().ptr(), m_unpackBuffer.read().ptr(), ret);
	result = tmpBuf;
	return result;
}

Variant godot::Sproto::protocol(const Variant name)
{
	Variant result;
	int tag;
	const char * pname;
	struct sproto_type * request;
	struct sproto_type * response;
	if (m_sproto == NULL) {
		return result;
	}
	if (name.get_type() == Variant::INT) {
		tag = name.operator signed int();
		pname = sproto_protoname(m_sproto, tag);
		if (pname == NULL) {
			return result;
		}
	}
	else if (name.get_type() == Variant::STRING) {
		pname = name.operator godot::String().utf8().get_data();
		if (pname == NULL) {
			return result;
		}
		tag = sproto_prototag(m_sproto, pname);
		if (tag < 0) {
			return result;
		}
	}
	else {
		return result;
	}
	result = Dictionary();
	result.operator godot::Dictionary()["pname"] = String(pname);
	result.operator godot::Dictionary()["tag"] = tag;

	request = sproto_protoquery(m_sproto, tag, SPROTO_REQUEST);
	if (request != NULL) {
		result.operator godot::Dictionary()["request"] = String(pname) + ".request";
	}

	response = sproto_protoquery(m_sproto, tag, SPROTO_RESPONSE);
	if (response == NULL) {
		result.operator godot::Dictionary()["response"] = String(pname) + ".response";
	}

	return result;
}

Variant godot::Sproto::test(const String name, const Dictionary data)
{
	Variant result;
	struct sproto_type * st = get_sproto_type(name);
	if (st == NULL) {
		Godot::print_error("this sproto type is NULL", "encode", __FILE__, __LINE__);
		return result;
	}

	struct encode_ud self;
	memset(&self, 0x00, sizeof(struct encode_ud));
	self.tbl = Variant(data);

	result = m_encodeBuffer;
	result.operator godot::PoolByteArray().resize(24);
	return result;
}


void Sproto::_init()
{
	printf("sproto::_init()\n");
}

void Sproto::_register_methods() {
	register_method("newsproto", &Sproto::newsproto);
	
	register_method("dumpsproto", &Sproto::dumpsproto);
	
	register_method("encode", &Sproto::encode);
	
	register_method("decode", &Sproto::decode);
	
	register_method("pack", &Sproto::pack);

	register_method("unpack", &Sproto::unpack);

	register_method("protocol", &Sproto::protocol);

	register_method("test", &Sproto::test);
	
}
