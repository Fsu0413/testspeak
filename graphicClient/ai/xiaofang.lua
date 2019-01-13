
local AiCommon = require("AiCommon")

data = {
	["recvList"] = {},
	["lastSent"] = {},
	["recvContent"] = {},

	["outoftime"] = false,
	["outoftimeDay"] = 0,
	["outoftimeKnown"] = {},

	["sendingStep"] = 0,
	["sending"] = "",
	["typed"] = "",
	["sendingTo"] = "",

	["sendpressed"] = false,
	["toview"] = {},
	["currentViewing"] = {},
}

consts = {
	["operationTimerId"] = AiCommon.UserTimerId + 3,

	["outoftimeTimerId"] = AiCommon.UserTimerId + 4,
	["outoftimeTimeout"] = 1000,

	["thinkdelay"] = 5000,
	["clickDelay"] = 200,
	["typeDelay"] = 100,
	["sendDelay"] = 1500,

	["boringKey"] = {
		"无聊",
		"难受",
		"寂寞",
		"一个人",
		"聊天",
		"撩",
		"尬"
	},

}

base = {
	["senddup"] = {
		"考考你，我叫什么来着？",
		"我多大了？",
		"还记得我是男生还是女生么？"
	},
	["changemale"] = {
		"顺便跟你说说，俺是个帅哥！",
		"稍微向你透露下，我是男生",
	},
	["changefemale"] = {
		"偷偷的告诉你，信不信我是大美女哦~",
		"我是女生，对我说话要温柔些~",
	},
	["callme"] = {
		"叫我吗？",
		"您在找我么",
		"需不需要我给您排忧解难？",
	},
	["greetfemale"] = {
		"我是__AIREPLACE__，看您有点无聊，我来陪您好了",
		"这么无聊，怕不是有什么心事？",
	},
	["greetmale"] = {
		"俺是__AIREPLACE__，想俺了么？",
		"看您这么无聊，是不是想俺想到流鼻血。"
	},
	["parrotdup"] = {
		"你叫什么来着？",
		"你多大了？",
		"你是男生还是女生啊？"
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
	local timer = 100

	if data.sendingStep == 0 then
		if #data.toview ~= 0 then
			local toview = data.toview[1]
			data.currentViewing = toview
			data.sendingStep = 101
			timer = consts.sendDelay
			table.remove(data.toview, 1)
		end
	elseif data.sendingStep == 3 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			data.sending = ""
			data.typed = ""
			me:setText("")
			timer = consts.sendDelay
		else
			if data.sending == "" then
				data.sendingStep = 4
				timer = consts.sendDelay
			else
				data.typed = data.typed .. me:getFirstChar(data.sending)
				data.sending = me:removeFirstChar(data.sending)
				me:setText(data.typed)
				timer = consts.typeDelay
			end
		end
	elseif data.sendingStep == 4 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			data.sendingStep = 0
			data.sending = ""
			data.typed = ""
			me:setText("")
			timer = consts.sendDelay
		else
			me:sendPress()
			data.sendpressed = true
			data.sendingStep = 5
			timer = consts.clickDelay
		end
	elseif data.sendingStep == 5 then
		if data.currentViewing.cancel then
			data.currentViewing = {}
			me:sendRelease()
			data.sendingStep = 0
			data.sending = ""
			data.typed = ""
			me:setText("")
			timer = consts.sendDelay
		else
			me:sendClick()
			data.sendpressed = false
			data.sendingStep = 0
			data.currentViewing = {}
			timer = consts.sendDelay
		end
	elseif data.sendingStep == 101 then
		data.sendingStep = 102
		me:setNameCombo(data.currentViewing.name)
		timer = consts.sendDelay
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
			timer = consts.sendDelay
		elseif data.currentViewing.content then
			data.sending = data.currentViewing.content
			data.typed = ""
			me:setTextFocus()
			data.sendingStep = 3
			timer = consts.thinkdelay
		end
	end

	timer = generateRandom(timer)

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

send = function(from, content)
	me:debugOutput("send" .. from .. content)
	data.lastSent[from] = content
	sendTo(from, content)
end

analyzeContent = function(from)
	if data.recvList[from][#(data.recvList[from])] == data.lastSent[from] then
		send(from, getStringFromBase("parrotdup"))
		return
	end
	me:debugOutput("data.queryTl" .. from .. data.recvList[from][#(data.recvList[from])])
	me:queryTl(from, data.recvList[from][#(data.recvList[from])])
	return true
end

talk = function(from, content)
	if not data.recvList[from] then
		data.recvList[from] = {}
	end

	table.insert(data.recvList[from], content)
	if #(data.recvList[from]) > 3 then
		table.remove(data.recvList[from], 1)
	end

	cancelAllPendingSend(from)

	if (data.sendingStep ~= 102) and (data.currentViewing.name == from) then
		data.currentViewing.cancel = true
	end

local judgeIgnore = function()
	if #(data.recvList[from]) == 3 then
		local allequal = true;
		local first
		for _, i in ipairs(data.recvList[from]) do
			if not first then
				first = i
			elseif first ~= i then
				allequal = false
				break
			end
		end
		if allequal then return end
	end

	return true
end

	if (data.sendingStep == 102) and (data.currentViewing.name == from) and (not data.currentViewing.cancel) then
		if not judgeIgnore() then
			data.currentViewing.cancel = true
		else
			return analyzeContent(from)
		end
	else
		if judgeIgnore() then
			return analyzeContent(from)
		end
	end
end

AiCommon.Callbacks.removePlayer = function(name)
	me:debugOutput("removePlayer"..name)
	data.recvList[name] = nil
	data.lastSent[name] = nil
	data.recvContent[name] = nil

	if data.currentViewing.name == name then
		data.currentViewing.cancel = true
	end

	cancelAllPendingSend(name)
end

AiCommon.Callbacks.messageReceived = function(from)
	me:debugOutput("messageReceived"..from)
	table.insert(data.toview, {["name"]=from})
end

AiCommon.Callbacks.messageDetail = function(detail)
local playerSpoken1 = function(from, content, fromYou, toYou, groupsent, sendtime)
	me:debugOutput("playerSpoken"..from..content)
	if fromYou then return end

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

	if toYou then
		local r = talk(from, content)
		return from, r
	end

	if groupsent then
		if string.find(content, me:name()) then
			send(from, getStringFromBase("callme"))
			return from
		else
			local boring = false
			for _, i in ipairs(consts.boringKey) do
				if string.find(content, i) then
					boring = true
					break
				end
			end

			if boring then
				local sending = getStringFromBase("greet" .. me:gender())
				sending = string.gsub(sending, "__AIREPLACE__", me:name())
				send(from, sending)
				return from
			end
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
		toSend = string.gsub(toSend, "%s", "")
		if toSend == data.lastSent[from] then
			toSend = getStringFromBase("senddup")
		end
	elseif value == 40004 then
		toSend = getStringFromBase("outoftime")
		if not data.outoftime then
			data.outoftimeDay = os.date("*t").day
			me:addTimer(consts.outoftimeTimerId, consts.outoftimeTimeout)
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
		toSend = getStringFromBase("change" .. me:gender())
	end
	return toSend
end
	local toSend = tlReceive1(value, sending, from)
	if toSend then
		send(from, toSend)
	elseif (data.sendingStep == 102) and (data.currentViewing.name == from) and (not data.currentViewing.cancel) then
		data.currentViewing.cancel = true
	end
end

timeout = function(timerid)
	if (timerid == consts.operationTimerId) then
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
		else
			me:addTimer(consts.outoftimeTimerId, consts.outoftimeTimeout)
		end
	else
		AiCommon.timeout(timerid)
	end
end

math.randomseed(os.time())
me:addTimer(consts.operationTimerId, 100)
