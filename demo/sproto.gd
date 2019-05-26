
const SprotoLib = preload("res://bin/sproto.gdns")
onready var sprotolib = SprotoLib.new()

var packagename = "package"
var protocol_cache = {}
var session_requests = {}

func _ready():
	pass
	
	
func pack(buffer):
	return sprotolib.pack(buffer)
	
func unpack(buffer):
	return sprotolib.unpack(buffer)

	
func encode(typename, tbl):
	return sprotolib.encode(typename, tbl)
	
func decode(typename, buffer):
	var tmp = sprotolib.decode(typename, buffer)
	if tmp == null:
		return null
	if typeof(tmp) == TYPE_ARRAY:
		return tmp[0]
	
func pencode(typename, tbl):
	var buffer = encode(typename, tbl)
	return pack(buffer)
	
func pdecode(typename, buffer):
	var tmpbuf = unpack(buffer)
	return decode(typename, tmpbuf)	

func queryproto(protoname):
	var v = protocol_cache[protoname]
	if v == null:
		v = sprotolib.protocol(protoname)
		if v != null and v.has("tag") and v.has("pname"):
			protocol_cache[v.tag] = v
			protocol_cache[v.pname] = v
	
	return v

func request_encode(protoname, tbl):
	var p = queryproto(protoname)
	if p.has("request"):
		return encode(p.request, tbl)
	else:
		return null

func response_encode(protoname, tbl):
	var p = queryproto(protoname)
	if p.has("response"):
		return encode(p.response, tbl)
	else:
		return null
	
func request_decode(protoname, buffer):
	var p = queryproto(protoname)
	if p.has("request"):
		return decode(p.request, buffer)
	else:
		return null
	
func response_decode(protoname, buffer):
	var p = queryproto(protoname)
	if p.has("response"):
		return decode(p.response, buffer)
	else:
		return null
		
		
class CbFunc:
	var sp = null
	var response_type = null
	var session = null
	var tag = null
	var package = null
	func _init(spr, type, sess, resp, pkgname):
		sp = spr
		tag = type
		session = sess
		response_type = resp
		package = pkgname
		pass
		
	func response(args, ud):
		var header_tmp = {}
		header_tmp.type = tag
		header_tmp.session = session
		header_tmp.ud = ud
		var buffer = sp.encode(package, args)
		if response_type != null:
			var content = sp.encode(response_type, args)
			var tmp = PoolByteArray(buffer).append_array(PoolByteArray(content))
			return sp.pack(Array(tmp))
		else:
			return sp.pack(buffer)


func gen_response(response, session, type):
	var cb = CbFunc.new(self, type, session, response, packagename)
	return funcref(cb, "response")
	
func dispatch(buffer):
	var bin = unpack(buffer)
	var header_ret = sprotolib.decode(packagename, bin)
	if header_ret == null:
		return
	var header_tmp = header_ret[0]
	var header_sz = header_ret[1]
	var tmp_buf = PoolByteArray(buffer)
	var left_buf = tmp_buf.subarray(0, header_sz)
	var content = Array(left_buf)
	var ud = null
	if header_tmp.has("ud"):
		ud = header_tmp.ud
	if header_tmp.has("type") and header_tmp.has("session"):
		var proto = queryproto(header_tmp.type)
		var result
		if proto.has("request"):
			result = decode(proto.request, content)
		if header_tmp.session > 0:
			return ["REQUEST", proto.name, result, gen_response(proto.response, header_tmp.session, null), ud]
		else:
			return ["REQUEST", proto.name, result, null, ud]
			
	elif not header_tmp.has("type") and header_tmp.has("session"):
		var protoname = session_requests[header_tmp.session]
		var proto = queryproto(protoname)
		session_requests.earse(header_tmp.session)
		if proto.has("response"):
			var result = decode(proto.response, content)
			return ["RESPONSE", protoname, result, header_tmp.session, ud]
		else:
			return ["RESPONSE", protoname, null, header_tmp.session, ud]
	elif header_tmp.has("type") and not header_tmp.has("session"):
		var proto = queryproto(header_tmp.type)
		if proto.has("response"):
			var result = decode(proto.response, content)
			return ["RESPONSE", proto.name, result, null, ud]
		else:
			var result = decode(proto.response, content)
			return ["RESPONSE", proto.name, null, null, ud]
	else:
		return null
		
func request(name, args, session, ud = null):
	var proto = queryproto(name)
	var header_tmp = {}
	header_tmp.type = proto.tag
	header_tmp.session = session
	header_tmp.ud = ud
	var buffer = encode(packagename, header_tmp)
	if session != null:
		session_requests[session] = proto.name
	if proto.has("request"):
		var content = encode(proto.request, args)
		return pack(buffer.concat(content))
	else:
		return pack(buffer)
	
func attach(pkgname):
	packagename = pkgname
	