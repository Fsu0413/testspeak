
data = {
	["speakingTo"] = "",
	["groupSpoken"] = {},
	["spokenToMe"] = {},

	["lastSent"] = "",
	["lastRecv"] = "",

	["outoftimeRegistered"] = false,

	["timeoutTime"] = 0,

	["sendingStep"] = 0,
	["sending"] = "",
	["typed"] = "",
	["sendingTo"] = "",

	["tosend"] = {},
	["sendpressed"] = false,
}

consts = {
	["findPersonTimerId"] = 1,
	["findPersonTimeout"] = 200000,

	["timeoutTimerId"] = 2,
	["timeoutTimeout"] = 300000,

	["operationTimerId"] = 3,

	["outoftimeTimerId"] = 4,
	["outoftimeTimeout"] = 100000,

	["thinkdelay"] = 500,
	["clickDelay"] = 200,
	["typeDelay"] = 100,
	["sendDelay"] = 1500,
}

base = {
	["senddup"] = {
		"感觉自己有点啰嗦了呢～",
		"好像不知道说点什么好了。。。",
		"我要开始复读机模式了"
	},
	["change"] = {
		"我靠，你能不能说点我能听得懂的",
		"噫"
	},
	["greet"] = {
		"在吗",
		"你好，我是__AIREPLACE__",
		"hello",
		"hi"
	},
	["parrotdup"] = {
		"学我说话有意思么。。。。",
		"我可能在和鹦鹉说话",
		"就这个事，不用重复啦！"
	},
	["recvdup"] = {
		"你是在自言自语吗？",
		"重要的事情不用说三遍啦。",
		"复读机，鉴定完毕。"
	},
	["gmale"] = {
		"本人性别男，爱好女，慢走不送"
	},
	["gfemale"] = {
		"我妈催我找对象，我着急找男性聊天，不好意思啦"
	},
	["findperson"] = {
		"无聊",
		"来个人陪陪我，否则我难受"
	},
	["timeout1"] = {
		"在想啥呢。。。",
		"嘿，醒醒"
	},
	["timeout2"] = {
		"掉线了么",
		"咋了，不理我了？"
	},
	["timeout3"] = {
		"看来真掉线了。我去找别人聊啦"
	},
	["outoftime"] = {
		"今天说了太多，次数用完啦。。。不得不下线了，明天继续！",
		"不好意思，没时间啦，明天有时间再聊~下线了，88~"
	},
}

generateRandom = function(rand)
	return math.random(rand * 0.3, rand * 1.5)
end

sendingstep = function()
	local timer = 1

	if (data.sendingStep ~= 5) and data.sendpressed then
		me:sendRelease()
	end

	if data.sendingStep == 0 then
		if #data.tosend ~= 0 then
			local tosend = data.tosend[1]
			data.sendingTo = tosend.to
			data.sending = tosend.content
			data.sendingStep = 1
			timer = consts.clickDelay
			table.remove(data.tosend, 1)
		end
	elseif data.sendingStep == 1 then
		-- me:popupNameCombo()
		data.sendingStep = 2
		timer = consts.clickDelay
	elseif data.sendingStep == 2 then
		me:setNameCombo(data.sendingTo)
		data.sendingStep = 3
		data.typed = ""
		timer = consts.thinkdelay
	elseif data.sendingStep == 3 then
		if data.sending == "" then
			data.sendingStep = 4
			timer = consts.sendDelay
		else
			data.typed = data.typed .. me:getFirstChar(data.sending)
			data.sending = me:removeFirstChar(data.sending)
			me:setText(data.typed)
			timer = consts.typeDelay
		end
	elseif data.sendingStep == 4 then
		me:sendPress()
		data.sendpressed = true
		data.sendingStep = 5
		timer = consts.clickDelay
	elseif data.sendingStep == 5 then
		me:sendClick()
		data.sendpressed = false
		data.sendingStep = 0
		timer = consts.thinkdelay
	end

	if timer ~= 1 then
		timer = generateRandom(timer)
	end

	me:addTimer(consts.operationTimerId, timer)
end

sendTo = function(to, content)
	if not to then to = "all" end
	me:debugOutput("sendTo".. to .. content)

	local tosend = {
		["to"] = to,
		["content"] = content
	}

	table.insert(data.tosend, tosend)
end

getStringFromBase = function(baseName)
	me:debugOutput("getStringFromBase" .. baseName)
	return base[baseName][math.random(1, #(base[baseName]))];
end

findPerson = function()
	me:debugOutput("findPerson")
	if data.speakingTo == "" then
		sendTo(nil, getStringFromBase("findperson"))
	end
	me:addTimer(consts.findPersonTimerId, generateRandom(consts.findPersonTimeout))
end

send = function(content)
	me:debugOutput("send" .. content)
	if data.speakingTo ~= "" then
		data.lastSent = content
		sendTo(data.speakingTo, content)
		me:addTimer(consts.timeoutTimerId, generateRandom(consts.timeoutTimeout))
	end
end

analyzeContent = function()
	if data.lastRecv == data.lastSent then
		send(getStringFromBase("parrotdup"))
		return
	end
	me:debugOutput("data.queryTl" .. data.speakingTo .. data.lastRecv)
	me:queryTl(data.speakingTo, data.lastRecv)
end

talk = function(content)
	me:killTimer(consts.timeoutTimerId)
	data.timeoutTime = 0
	if content == data.lastRecv then
		send(getStringFromBase("recvdup"))
	else
		data.lastRecv = content
		analyzeContent()
	end
end

addPlayer = function(name)

end

removePlayer = function(name)
	me:debugOutput("removePlayer"..name)
	if data.speakingTo == name then
		data.speakingTo = ""
		me:killTimer(consts.timeoutTimerId)
		data.timeoutTime = 0
	end

	if data.sendingTo == name then
		data.sending = ""
		data.sendingTo = ""
		data.sendingStep = 0
	end

	local flag = false
	while not flag do
		flag = true
		for _, i in ipairs(data.tosend) do
			if i.to == name then
				table.remove(data.tosend, _)
				flag = false
				break
			end
		end
	end
end

playerDetail = function(obname, obgender)
	me:debugOutput("playerDetail"..obname..obgender)
	if (data.spokenToMe[obname] ~= nil) and (data.speakingTo == "") then
		if (me:gender() ~= obgender) then
			data.speakingTo = obname
			talk(data.spokenToMe[obname])
			data.spokenToMe = {}
			data.groupSpoken = {}
		else
			sendTo(obname, getStringFromBase("g" .. me:gender()))
			data.spokenToMe[obname] = nil
		end
	else
		for _, n in ipairs(data.groupSpoken) do
			if (n == obname) and (data.speakingTo == "") then
				if (me:gender() ~= obgender) then
					local sending = getStringFromBase("greet")
					sending = string.gsub(sending, "__AIREPLACE__", me:name())
					sendTo(obname, sending)
				else
					table.remove(data.groupSpoken, _)
					break
				end
			end
		end
	end
end

playerSpoken = function(from, to, content, fromYou, toYou, groupsent)
	me:debugOutput("playerSpoken"..from..to..content)
	if fromYou then return end

	if groupsent and (data.speakingTo == "") then
		local flag = false
		for _, n in ipairs(data.groupSpoken) do
			if n == from then
				flag = true
				break
			end
		end
		if not flag then
			table.insert(data.groupSpoken, from)
		end
		me:queryPlayer(from)
	end

	if toYou and (data.speakingTo == "") then
		data.spokenToMe[from] = content
		me:queryPlayer(from)
	end

	if toYou and (from == data.speakingTo) then
		talk(content)
	end
end

tlReceive = function(value, sending)
	me:debugOutput("tlReceive" .. value .. sending)
	local toSend = ""
	if (value == 100000) or (value == 40002) then
		toSend = sending
		toSend = string.gsub(toSend, "%s", "")
		if toSend == data.lastSent then
			toSend = getStringFromBase("senddup")
		end
	elseif value == 40004 then
		toSend = getStringFromBase("outoftime")
		if not data.outoftimeRegistered then
			me:addTimer(consts.outoftimeTimerId, consts.outoftimeTimeout)
			data.outoftimeRegistered = true
		end
	else
		-- how to qDebug()????
	end

	if (toSend == "") then
		toSend = getStringFromBase("change")
	end

	send(toSend)
end

timeout = function(timerid)
	if timerid == consts.findPersonTimerId then
		me:debugOutput("findPersonTimerId")
		findPerson()
	elseif timerid == consts.timeoutTimerId then
		data.timeoutTime = data.timeoutTime + 1
		local sending = getStringFromBase("timeout" .. tostring(data.timeoutTime))
		send(sending)

		if data.timeoutTime >= 3 then
			data.speakingTo = ""
			me:killTimer(consts.timeoutTimerId)
		end
	elseif (timerid == consts.operationTimerId) then
		sendingstep()
	elseif (timerid == consts.outoftimeTimerId) then
		me:prepareExit()
	end
end

me:addTimer(consts.findPersonTimerId, 2000)
me:addTimer(consts.operationTimerId, 100)
