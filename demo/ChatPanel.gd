extends Panel

onready var _client = get_node("SkynetClient")
onready var _log_dest = get_node("VBoxContainer/RichTextLabel")
onready var _url_edit = get_node("VBoxContainer/Connect/LineEdit")
onready var _text_edit = get_node("VBoxContainer/SendText/LineEdit")
 
# Called when the node enters the scene tree for the first time.
func _ready():
	_client.connect("on_open", self, "_on_open")
	_client.connect("on_close", self, "_on_close")
	_client.connect("on_error", self, "_on_error")
	_client.connect("on_data", self, "_on_data")


# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass


func _on_open():
	pass

func _on_close():
	pass
	
func _on_error():
	pass
	
func _on_data(host, port, result):
	print("recv from server result=", result)
	_log_dest.add_text("recv a data from server")


func _on_send_pressed():
	if _text_edit.text == "":
		return
	var param = {
		"account" : "kuzhu1990",
		"password" : "123456",
		"platform" : "default"
	}
	var ext = {}
	var err = _client.send_data("account.register", param, ext)
	if err == OK:
		var msg = str("[Me] %s" % [ _text_edit.text ]) + "\n"
		_log_dest.add_text(msg)
	_text_edit.text = ""


func _on_connect_toggled(pressed):
	if pressed:
		if _url_edit.text != "":
			_client.connect_server(_url_edit.text)
	else:
		_client.disconnect_server()	
