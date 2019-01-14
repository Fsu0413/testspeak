
local AiCommon = require("AiCommon")

data = {
	["repeatTime"] = 0,
	["banned"] = {},

	["speakingTo"] = "",
	["groupSpoken"] = {},
	["recvContent"] = {},

	["lastSent"] = "",
	["lastRecv"] = "",

	["outoftime"] = false,
	["outoftimeDay"] = 0,
	["outoftimeKnown"] = {},

	["timeoutTime"] = 0,

	["sendingStep"] = 0,
	["sending"] = "",
	["typed"] = "",
	["sendingTo"] = "",

	["sendpressed"] = false,
	["toview"] = {},
	["currentViewing"] = {},
}

consts = {
	["findPersonTimerId"] = AiCommon.UserTimerId + 1,
	["findPersonTimeout"] = 200000,
	["timeoutTimerId"] = AiCommon.UserTimerId + 2,
	["timeoutTimeout"] = 300000,
	["operationTimerId"] = AiCommon.UserTimerId + 3,
	["outoftimeTimerId"] = AiCommon.UserTimerId + 4,
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
		"您好，我是__AIREPLACE__",
		"hello",
		"hi"
	},
	["parrotdup"] = {
		"学我说话有意思么。。。。",
		"我可能在和鹦鹉说话",
		"就这个事，不用重复啦！"
	},
	["recvdup"] = {
		"您是在自言自语吗？",
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
	["revive"] = {
		"我回来啦，有没有想我呢？",
		"嗨，我现在有时间啦，来聊两句？",
	},
}

sendingstep = function()
	local timer = 100

	if data.sendingStep == 0 then
		if #data.toview ~= 0 then
			local toview = data.toview[1]
			data.currentViewing = toview
			data.sendingStep = 101
			timer = AiCommon.TimerConsts.sendDelay
			table.remove(data.toview, 1)
		end
	elseif data.sendingStep == 3 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			data.sending = ""
			data.typed = ""
			me:setText("")
			timer = AiCommon.TimerConsts.sendDelay
		else
			if data.sending == "" then
				data.sendingStep = 4
				timer = AiCommon.TimerConsts.sendDelay
			else
				data.typed = data.typed .. me:getFirstChar(data.sending)
				data.sending = me:removeFirstChar(data.sending)
				me:setText(data.typed)
				timer = AiCommon.TimerConsts.typeDelay
			end
		end
	elseif data.sendingStep == 4 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			data.sending = ""
			data.typed = ""
			me:setText("")
			timer = AiCommon.TimerConsts.sendDelay
		else
			me:sendPress()
			data.sendpressed = true
			data.sendingStep = 5
			timer = AiCommon.TimerConsts.clickDelay
		end
	elseif data.sendingStep == 5 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			me:sendRelease()
			data.sendingStep = 0
			data.sending = ""
			data.typed = ""
			me:setText("")
			timer = AiCommon.TimerConsts.sendDelay
		else
			me:sendClick()
			data.sendpressed = false
			data.sendingStep = 0
			data.currentViewing = {}
			timer = AiCommon.TimerConsts.sendDelay
		end
	elseif data.sendingStep == 101 then
		data.sendingStep = 102
		me:setNameCombo(data.currentViewing.name)
		timer = AiCommon.TimerConsts.sendDelay
	elseif data.sendingStep == 102 then
		if not data.currentViewing.time then
			data.currentViewing.time = os.time()
		else
			local passedTime = os.difftime(os.time(), data.currentViewing.time)
			if passedTime >= 30 then
				data.currentViewing.cancel = true
			end
		end

		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			timer = AiCommon.TimerConsts.sendDelay
		elseif data.currentViewing.content then
			data.sending = data.currentViewing.content
			data.typed = ""
			me:setTextFocus()
			data.sendingStep = 3
			timer = AiCommon.TimerConsts.thinkDelay
		end
	end

	timer = AiCommon.generateRandom(timer)

	me:addTimer(consts.operationTimerId, timer)
end

cancelAllPendingSend = function(to)
	local exist = true
	while exist do
		exist = false
		for _, p in ipairs(data.toview) do
			if p.name == to then
				table.remove(data.toview, _)
				exist = true
				break
			end
		end
	end
end

sendTo = function(to, content)
	if not to then to = "all" end
	me:debugOutput("sendTo".. to .. content)

	if (data.currentViewing.name == to) and (data.sendingStep == 102) and (not data.currentViewing.cancel) then
		data.currentViewing.content = content
	else
		local x = {
			["name"] = to,
			["content"] = content,
		}
		table.insert(data.toview, x)
	end
end

getStringFromBase = function(baseName)
	me:debugOutput("getStringFromBase" .. baseName)
	return base[baseName][math.random(1, #(base[baseName]))];
end

findPerson = function()
	me:debugOutput("findPerson")
	if (data.speakingTo == "") and (not data.outoftime) then
		sendTo(nil, getStringFromBase("findperson"))
	end
	me:addTimer(consts.findPersonTimerId, AiCommon.generateRandom(consts.findPersonTimeout))
end

send = function(content)
	me:debugOutput("send" .. content)
	if data.speakingTo ~= "" then
		data.lastSent = content
		sendTo(data.speakingTo, content)
		if not data.outoftime then
			me:addTimer(consts.timeoutTimerId, AiCommon.generateRandom(consts.timeoutTimeout))
		end
	end
end

analyzeContent = function()
	if data.lastRecv == data.lastSent then
		send(getStringFromBase("parrotdup"))
		return
	end
	me:debugOutput("data.queryTl" .. data.speakingTo .. data.lastRecv)
	me:queryTl(data.speakingTo, data.lastRecv)
	return true
end

talk = function(content)
	me:killTimer(consts.timeoutTimerId)
	data.timeoutTime = 0

	cancelAllPendingSend(from)

	if (data.sendingStep ~= 102) and (data.currentViewing.name == from) then
		data.currentViewing.cancel = true
	end

	if content == data.lastRecv then
		send(getStringFromBase("recvdup"))
		data.repeatTime = data.repeatTime + 1
		if data.repeatTime == 5 then
			table.insert(data.banned, data.speakingTo)
			data.speakingTo = ""
			me:killTimer(consts.timeoutTimerId)
			data.timeoutTime = 0
		end
	else
		data.repeatTime = 0
		data.lastRecv = content
		return analyzeContent()
	end
end

AiCommon.Callbacks.removePlayer = function(name)
	me:debugOutput("removePlayer"..name)
	if data.speakingTo == name then
		data.speakingTo = ""
		me:killTimer(consts.timeoutTimerId)
		data.timeoutTime = 0
	end

	data.recvContent[name] = nil

	if data.currentViewing.name == name then
		data.currentViewing.cancel = true
	end

	cancelAllPendingSend(name)

	local flag = false
	while not flag do
		flag = true
		for _, i in ipairs(data.banned) do
			if i == name then
				table.remove(data.banned, _)
				flag = false
				break
			end
		end
	end
end

AiCommon.Callbacks.messageReceived = function(from)
	local toview = {["name"] = from}

	if from == "all" then
		if data.speakingTo == "" then
			table.insert(data.toview, 1, toview)
		else
			table.insert(data.toview, toview)
		end
	else
		if from == data.speakingTo then
			table.insert(data.toview, 1, toview)
		else
			table.insert(data.toview, toview)
		end
	end
end

AiCommon.Callbacks.messageDetail = function(detail)
local playerSpoken1 = function(from, content, fromYou, toYou, groupsent, sendtime)
	me:debugOutput("playerSpoken"..from..content)
	if fromYou then return end

	for _, i in ipairs(data.banned) do
		if i == from then
			return
		end
	end

	local relatedPerson = from
	if groupSent then
		relatedPerson = "all"
	end
	local recvContent = data.recvContent[relatedPerson]
	if recvContent and (recvContent.time == sendtime) and (recvContent.content == content) then
		return
	end
	data.recvContent[relatedPerson] = {
		["time"] = sendtime,
		["content"] = content
	}

	for _, i in ipairs(data.outoftimeKnown) do
		if i == from then
			return
		end
	end

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
			local sending = getStringFromBase("greet")
			sending = string.gsub(sending, "__AIREPLACE__", me:name())
			sendTo(from, sending)
			return from
		end
	elseif toYou then
		if data.speakingTo == "" then
			data.speakingTo = from
			data.groupSpoken = {}
			local r = talk(content)
			return from, r
		elseif from == data.speakingTo then
			local r = talk(content)
			return from, r
		end
	end
end
	if (data.sendingStep == 102) and ((data.currentViewing.name == detail.from) or (data.currentViewing.name == "all")) and (not data.currentViewing.cancel) then
		local willSpeak, async = playerSpoken1(detail.from, detail.content, detail.fromYou, detail.toYou, detail.groupSent, detail.time)
		if not willSpeak then
			if not data.currentViewing.content then
				data.currentViewing.cancel = true
			end
		else
			if data.currentViewing.content and async then
				data.currentViewing.content = nil
			end
			if willSpeak ~= data.currentViewing.name then
				data.currentViewing.cancel = true
			end
		end
	else
		playerSpoken1(detail.from, detail.content, detail.fromYou, detail.toYou, detail.groupSent, detail.time)
	end
end

tlReceive = function(value, sending, from)
local tlReceive1 = function(value, sending, from)
	me:debugOutput("tlReceive" .. value .. sending)
	local toSend = ""
	if (value == 100000) or (value == 40002) then
		toSend = sending
		toSend = string.gsub(toSend, "%s+", " ")
		if toSend == data.lastSent then
			toSend = getStringFromBase("senddup")
		end
	elseif value == 40004 then
		toSend = getStringFromBase("outoftime")
		if not data.outoftime then
			data.outoftimeDay = os.date("*t").day
			me:addTimer(consts.outoftimeTimerId, AiCommon.TimerConsts.outoftimeTimeout)
			data.outoftime = true
		end

		for _, i in ipairs(data.outoftimeKnown) do
			if i == from then
				return
			end
		end
		table.insert(data.outoftimeKnown, from)
	else
		-- how to qDebug()????
	end

	if (toSend == "") then
		toSend = getStringFromBase("change")
	end

	return toSend
end
	local toSend = tlReceive1(value, sending, from)
	if toSend then
		send(toSend)
	elseif (data.sendingStep == 102) and (data.currentViewing.name == from) and (not data.currentViewing.cancel) then
		data.currentViewing.cancel = true
	end
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
		local dt = os.date("*t")
		me:debugOutput("outoftimeTimerId " .. dt.day .. " " .. dt.hour .. " " .. dt.min.. " " .. dt.sec)
		local revive = false
		if dt.day ~= data.outoftimeDay then
			if dt.hour >= 1 then
				revive = true
			elseif dt.min > 55 then
				revive = true
			elseif dt.min > 5 then
				revive = (math.random(500) == 1)
			end
		end

		if revive then
			data.outoftime = false
			data.outoftimeDay = 0
			data.outoftimeKnown = {}
			if data.speakingTo ~= "" then
				send(getStringFromBase("revive"))
			end
		else
			me:addTimer(consts.outoftimeTimerId, AiCommon.TimerConsts.outoftimeTimeout)
		end
	else
		AiCommon.timeout(timerid)
	end
end

math.randomseed(os.time())
me:addTimer(consts.findPersonTimerId, 2000)
me:addTimer(consts.operationTimerId, 100)
