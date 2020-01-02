extends Node

#var Sproto = preload("res://sproto.gd")
#var sproto = Sproto.new()

onready var _sproto = get_node("sproto")

var _client = WebSocketClient.new()
var _fmt = WebSocketPeer.WRITE_MODE_BINARY
var last_connected_client = 0
signal on_open
signal on_data
signal on_close
signal on_error

# Called when the node enters the scene tree for the first time.
func _ready():
	var f = File.new()
	if (f.open("res://protocol.spb", File.READ)) == OK:
		var buffer = f.get_buffer(f.get_len())
		var tmp = Array(buffer)
		_sproto.loadsproto(tmp)
		f.close()
	_client.connect("connection_established", self, "_on_connection_established")
	_client.connect("connection_error", self, "_on_connection_error")
	_client.connect("connection_closed", self, "_on_connection_closed")
	_client.connect("data_received", self, "_on_data_received")
	

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	_client.poll()


func _on_connection_established(protocol):
	_client.get_peer(1).set_write_mode(_fmt)
	emit_signal("on_open")
	
func _on_connection_closed():
	emit_signal("on_close")
	set_process(false)
	
func _on_connection_error():
	emit_signal("on_error")
	set_process(false)
	
	
func _on_data_received():
	var bytes = _client.get_peer(1).get_packet()
	var host = _client.get_peer(1).get_connected_host()
	var port = _client.get_peer(1).get_connected_port()
	#var msg = packet.get_string_from_utf8()
	
	var result = _sproto.dispatch(bytes)
	
	emit_signal("on_data", host, port, result)
	
	
func connect_server(host):
	var err = _client.connect_to_url(host)
	if err != OK:
		set_process(false)
	else:
		set_process(true)

func disconnect_server():
	_client.disconnect_from_host()
	set_process(false)
	
func send_data(method, param, ext):	
	var bytes = _sproto.request("account.register", param, ext)
	var err = _client.get_peer(1).put_packet(bytes)
	return err
