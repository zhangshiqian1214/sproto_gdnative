extends Node2D

# Declare member variables here. Examples:
# var a = 2
# var b = "text"

const Sproto = preload("res://bin/sproto.gdns")
onready var sproto = Sproto.new()

# Called when the node enters the scene tree for the first time.
func _ready():		
	var _client = WebSocketClient.new()

# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass


func _on_Button_pressed():
	var f = File.new()
	if (f.open("res://protocol.spb", File.READ)) == OK:
		var buffer = f.get_buffer(f.get_len())
		var tmp = Array(buffer)
		sproto.newsproto(tmp)
		f.close()


func _on_Button2_pressed():
	sproto.dumpsproto()

func _on_Button3_pressed():
	var player = {
		"playerid" : 135469,
		"nickname" : "张三",
		"headid" : 1001,
		"headurl" : "http://img.52z.com/upload/news/image/20181108/20181108204521_83402.jpg",
		"sex":0,
		"isvip":true,
		"gold":10003801,
		"signs": [false, true, true, true, true, false],
		"pets":[13001, 13002, 13003, 13004],
		"mails":["tx 第一个", "alibaba 第二", "wangyi 第三"],
		"friends": {
			"zhangsan" : { "playerid": 13001, "nickname": "zhangsan" },
			"lishi" : { "playerid": 13002, "nickname": "lishi" },
			"lihui" : { "playerid": 13003, "nickname": "lihui" },
			"wangwu" : { "playerid": 13004, "nickname": "wangwu" },
		},
		"money":123.87,
		"master": { "playerid": 13004, "nickname": "wangwu" }
	}
	
	var unixdt = OS.get_unix_time()
# warning-ignore:unused_variable
	for i in range(500000):
		var bytes = sproto.encode("auth.Player", player)
		var result = sproto.decode("auth.Player", bytes)
#		print(result)
	
	var enddt = OS.get_unix_time()
	var interval = enddt - unixdt
	print("用时:", interval)
	#print("result=", result)


func _on_Button4_pressed():
	var player = {
		"playerid" : 135469,
		"nickname" : "张三",
		"headid" : 1001,
		"headurl" : "http://img.52z.com/upload/news/image/20181108/20181108204521_83402.jpg",
		"sex":0,
		"isvip":true,
		"gold":10003801,
		"signs": [false, true, true, true, true, false],
		"pets":[13001, 13002, 13003, 13004],
		"mails":["tx 第一个", "alibaba 第二", "wangyi 第三"],
		"friends": {
			"zhangsan" : { "playerid": 13001, "nickname": "zhangsan" },
			"lishi" : { "playerid": 13002, "nickname": "lishi" },
			"lihui" : { "playerid": 13003, "nickname": "lihui" },
			"wangwu" : { "playerid": 13004, "nickname": "wangwu" },
		},
		"money":123.87,
		"master": { "playerid": 13004, "nickname": "wangwu" }
	}
	
	
	for i in range(10000):
		var buffer = Array()
		buffer.resize(1024)
		sproto.test("auth.player", buffer)
	
