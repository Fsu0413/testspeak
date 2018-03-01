
local AiCommon = {}


AiCommon.UserTimerId = 100
AiCommon.Callbacks = {}

local data = {
	onlinePlayers = {},
	newMessagePlayers = {},
	newestMessage = {},
}


local consts = {
	["GetNewestInfoTimerId"] = 1,
	["GetNewestInfoTimeout"] = 1,
}

local callAddPlayer = function(name)
	me:debugOutput("addPlayer" .. name)
	local x = AiCommon.Callbacks.addPlayer
	if x and (type(x) == "function") then
		x(name)
	end
end

local callRemovePlayer = function(name)
	me:debugOutput("removePlayer" .. name)
	local x = AiCommon.Callbacks.removePlayer
	if x and (type(x) == "function") then
		x(name)
	end
end

local callMessageReceived = function(name)
	me:debugOutput("messageReceived" .. name)
	local x = AiCommon.Callbacks.messageReceived
	if x and (type(x) == "function") then
		x(name)
	end
end

local callMessageDetail = function(name)
	me:debugOutput("callMessageDetail" .. name.from)
	local x = AiCommon.Callbacks.messageDetail
	if x and (type(x) == "function") then
		x(name)
	end
end

local messageEqual = function(toJudge)
	if data.newestMessage.from ~= toJudge.from then return false end
	if data.newestMessage.fromYou ~= toJudge.fromYou then return false end
	if data.newestMessage.toYou ~= toJudge.toYou then return false end
	if data.newestMessage.groupSent ~= toJudge.groupSent then return false end
	if data.newestMessage.time ~= toJudge.time then return false end
	if data.newestMessage.content ~= toJudge.content then return false end
	
	return true
end

local getNewestInfo = function()
	local onlinePlayers = me:onlinePlayers()
	for _, x in ipairs(onlinePlayers) do
		if x ~= "all" then
			local contains = false
			for _, y in ipairs(data.onlinePlayers) do
				if x == y then
					contains = true
					break
				end
			end
			
			if not contains then
				callAddPlayer(x)
			end
		end
	end
	
	for _, y in ipairs(data.onlinePlayers) do
		if y ~= "all" then
			local contains = false
			for _, x in ipairs(onlinePlayers) do
				if x == y then
					contains = true
					break
				end
			end
			
			if not contains then
				callRemovePlayer(y)
			end
		end
	end
	
	data.onlinePlayers = onlinePlayers
	
	local newMessagePlayers = me:newMessagePlayers()
	for _, x in ipairs(newMessagePlayers) do
		local contains = false
		for _, y in ipairs(data.newMessagePlayers) do
			if x == y then
				contains = true
				break
			end
		end
		
		if not contains then
			callMessageReceived(x)
		end
	end
	
	data.newMessagePlayers = newMessagePlayers
	
	local newestSpokenMesage = me:getNewestSpokenMessage()
	if not messageEqual(newestSpokenMesage) then
		if newestSpokenMesage.content ~= "" then
			callMessageDetail(newestSpokenMesage)
		end
	end
	
	data.newestMessage = newestSpokenMesage
end


AiCommon.timeout = function(timerId)
	if timerId == consts.GetNewestInfoTimerId then
		getNewestInfo()
		me:addTimer(consts.GetNewestInfoTimerId, consts.GetNewestInfoTimeout)
	end
end


addPlayer = function()

end

removePlayer = function()

end

playerDetail = function()

end

playerSpoken = function()

end

me:addTimer(consts.GetNewestInfoTimerId, consts.GetNewestInfoTimeout)


return AiCommon