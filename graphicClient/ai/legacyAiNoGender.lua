
local AiCommon = require("AiCommon")

local localFunc = {}

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
	["findPersonTimeout"] = 600000,
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
	elseif data.sendingStep == 101 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
		else
			data.sendingStep = 102
			me:setNameCombo(data.currentViewing.name)
		end
	elseif data.sendingStep == 102 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			timer = AiCommon.TimerConsts.sendDelay
		else
			if not data.currentViewing.time then
				data.currentViewing.time = os.time()
				localFunc.messageDetail1(me:getNewestSpokenMessage())
			elseif data.currentViewing.content then
				local passedTime = os.difftime(os.time(), data.currentViewing.time)
				data.sendingStep = 103
				timer = math.max(AiCommon.TimerConsts.thinkDelay - passedTime * 1000 - AiCommon.TimerConsts.sendDelay, 100)
			else
				local passedTime = os.difftime(os.time(), data.currentViewing.time)
				if passedTime >= 30 then
					data.currentViewing.cancel = true
				end
			end
		end
	elseif data.sendingStep == 103 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
		else
			data.sending = data.currentViewing.content
			data.typed = ""
			me:setTextFocus()
			data.sendingStep = 3
			timer = AiCommon.TimerConsts.sendDelay
		end
	elseif data.sendingStep == 3 then
		if data.sending == "" then
			if data.currentViewing.cancel then
				data.sendingStep = 77
				timer = AiCommon.TimerConsts.cancelDelay
			else
				data.sendingStep = 4
				timer = AiCommon.TimerConsts.sendDelay
			end
		else
			if data.currentViewing.cancel then
				data.sendingStep = 76
			end

			data.typed = data.typed .. me:getFirstChar(data.sending)
			data.sending = me:removeFirstChar(data.sending)
			me:setText(data.typed)
			timer = AiCommon.TimerConsts.typeDelay
		end
	elseif data.sendingStep == 4 then
		if data.currentViewing.cancel then
			data.sendingStep = 77
			timer = AiCommon.TimerConsts.cancelDelay
		else
			me:sendPress()
			data.sendpressed = true
			data.sendingStep = 5
			timer = AiCommon.TimerConsts.clickDelay
		end
	elseif data.sendingStep == 5 then
		if data.currentViewing.cancel then
			me:sendRelease()
			data.sendingStep = 77
			timer = AiCommon.TimerConsts.cancelDelay
		else
			me:sendClick()
			data.sendingStep = 0
			data.currentViewing = {}
			timer = AiCommon.TimerConsts.sendDelay
		end
		data.sendpressed = false
	elseif data.sendingStep == 76 then
		if (data.sending == "") or (math.random(0, 9) < 1) then
			data.sendingStep = 77
			timer = AiCommon.TimerConsts.cancelDelay
		else
			data.typed = data.typed .. me:getFirstChar(data.sending)
			data.sending = me:removeFirstChar(data.sending)
			me:setText(data.typed)
			timer = AiCommon.TimerConsts.typeDelay
		end
	elseif data.sendingStep == 77 then
		data.sending = ""
		data.typed = ""
		me:setTextFocus()
		data.sendingStep = 78
		timer = AiCommon.TimerConsts.sendDelay
	elseif data.sendingStep == 78 then
		me:setText("")
		data.currentViewing = {}
		data.sendingStep = 0
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

sendTo = function(to, content, isFromTl, isForcedSendToAll)
	if not to then to = "all" end
	me:debugOutput("sendTo".. to .. content)
	cancelAllPendingSend(to)

	if (data.currentViewing.name == to) and (data.sendingStep == 102) and (not data.currentViewing.cancel) then
		data.currentViewing.content = content
		data.currentViewing.contentIsFromTl = isFromTl
	else
		local x = {
			["name"] = to,
			["content"] = content,
			["contentIsFromTl"] = isFromTl,
			["forcedSendToAll"] = isForcedSendToAll,
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
		sendTo(nil, getStringFromBase("findperson"), false, true)
	end
	me:addTimer(consts.findPersonTimerId, AiCommon.generateRandom(consts.findPersonTimeout))
end

send = function(content, isFromTl)
	me:debugOutput(tostring(isFromTl) .. "send" .. content)
	if data.speakingTo ~= "" then
		data.lastSent = content
		sendTo(data.speakingTo, content, isFromTl)
		if not data.outoftime then
			me:addTimer(consts.timeoutTimerId, AiCommon.generateRandom(consts.timeoutTimeout))
		end
	end
end

analyzeContent = function()
	if data.lastRecv == data.lastSent then
		send(getStringFromBase("parrotdup"), false)
		return
	end
	me:debugOutput("data.queryTl" .. data.speakingTo .. data.lastRecv)
	me:queryTl(data.speakingTo, data.lastRecv)
	return true
end

talk = function(content)
	me:killTimer(consts.timeoutTimerId)
	data.timeoutTime = 0

	if (data.sendingStep ~= 102) and (data.currentViewing.name == from) then
		data.currentViewing.cancel = true
	end

	if content == data.lastRecv then
		send(getStringFromBase("recvdup"), false)
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
end

local messageReceived = function(from)
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

AiCommon.Callbacks.messageReceived = function(from)
	me:debugOutput("messageReceived"..from)
	messageReceived(from)
end

AiCommon.Callbacks.messageDetail = function(detail)
	if detail.fromYou then
		return
	end

	if (not data.currentViewing.name)
			or ((data.currentViewing.name == "all") and detail.groupSent)
			or ((data.currentViewing.name == detail.from) and (not detail.groupSent)) then
		messageReceived(detail.groupSent and "all" or detail.from)
	end
end

--AiCommon.Callbacks.messageDetail = function(detail)
localFunc.messageDetail1 = function(detail)
local playerSpoken1 = function(from, content, fromYou, toYou, groupsent, sendtime)
	me:debugOutput("playerSpoken"..from..content)

	if fromYou or (sendtime == 0) then
		return
	end

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
			sendTo(from, sending, false)
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
			if (willSpeak ~= data.currentViewing.name) and (not (data.currentViewing.forcedSendToAll and data.currentViewing.name == "all")) then
				data.currentViewing.cancel = true
			end
		end
	end
end

tlReceive = function(value, sending, from)
local tlReceive1 = function(value, sending, from)
	local isFromTl = false
	me:debugOutput("tlReceive" .. value .. sending)
	local toSend = ""
	if (value == 100000) or (value == 40002) then
		toSend = sending
		if toSend == data.lastSent then
			toSend = getStringFromBase("senddup")
		else
			isFromTl = true
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

	return toSend, isFromTl
end
	local toSend, isFromTl = tlReceive1(value, sending, from)
	if toSend then
		send(toSend, isFromTl)
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
		send(sending, false)

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
				send(getStringFromBase("revive"), false)
			end
		else
			me:addTimer(consts.outoftimeTimerId, AiCommon.TimerConsts.outoftimeTimeout)
		end
	else
		AiCommon.timeout(timerid)
	end
end

math.randomseed(os.time())
me:addTimer(consts.operationTimerId, 100)

findPerson()
