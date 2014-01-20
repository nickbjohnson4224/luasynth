function signal(func, duration)
	local self = {}
	local mt = {}

	if duration == nil then
		if func ~= nil then
			duration = math.huge
		else
			duration = 0
		end
	end

	if type(func) == 'number' then
		local const = func
		func = function(t) return const end
	elseif
		type(func) == 'nil' then
		func = function(t) return 0 end
	end

	self.duration = duration

	function mt.__call(this, t)
		if type(t) == 'number' then
			if t > 0 and t < this.duration then
				return func(t)
			else
				return 0.0
			end
		else
			return signal(function(t1) return func(t(t1)) end)
		end
	end

	function mt.__add(l, r)
		
		if type(l) == 'number' then
			if l == 0 then
				return r
			elseif type(r) == 'number' then
				return l + r
			else
				return signal(function(t) return l + r(t) end)
			end
		else
			if type(r) == 'number' then
				return signal(function(t) return l(t) + r end)
			else
				return signal(function(t) return l(t) + r(t) end, math.max(l.duration, r.duration))
			end
		end

	end

	function mt.__minus(x)
		return signal(function(t) return -x(t) end, x.duration)
	end

	function mt.__sub(l, r)

		if type(l) == 'number' then
			if type(r) == 'number' then
				return l - r
			else
				return signal(function(t) return l - r(t) end)
			end
		else
			if type(r) == 'number' then
				return signal(function(t) return l(t) - r end)
			else
				return signal(function(t) return l(t) - r(t) end, math.max(l.duration, r.duration))
			end
		end

	end

	function mt.__mul(l, r)

		if type(l) == 'number' then
			if type(r) == 'number' then
				return l * r
			else
				return signal(function(t) return l * r(t) end, r.duration)
			end
		else
			if type(r) == 'number' then
				return signal(function(t) return l(t) * r end, l.duration)
			else
				return signal(function(t) return l(t) * r(t) end, math.min(r.duration, l.duration))
			end
		end

	end

	function mt.__div(l, r)

		if type(l) == 'number' then
			if type(r) == 'number' then
				return l / r
			else
				return signal(function(t) return l / r(t) end, r.duration)
			end
		else
			if type(r) == 'number' then
				return signal(function(t) return l(t) / r end, l.duration)
			else
				return signal(function(t) return l(t) / r(t) end, r.duration, l.duration)
			end
		end

	end

	function mt.__mod(l, r)

		if type(r) == 'number' then
			return signal(func, r)
		elseif type(r) == 'table' then
			return signal(
				function(t)
					return func(t + r[1])
				end, 
				r[2] - r[1]
			)
		end
		
	end

	function mt.__concat(l, r)
		if type(l) == 'nil' then
			return r
		elseif type(r) == 'nil' then
			return l
		else
			return signal(function(t)
				if t < l.duration then 
					return l(t)
				else
					return r(t - l.duration)
				end
			end, l.duration + r.duration)
		end
	end

	setmetatable(self, mt)
	return self
end

function tosignal(s)
	if type(s) == 'number' then return signal(s) end
	return s
end

--
-- Tools
--

function loop(s, count)

	if count == nil then
		count = math.huge
	end

	return signal(function(t)
		return s(t % s.duration)
	end)
end

--
-- Function signal wrappers
--

sin = signal(function(t)
	return math.sin(t)
end)

floor = signal(function(t)
	return math.floor(t)
end)

--
-- Tone generators
--

TIME = signal(function(t) return t end)

function sine_wave(f)

	if type(f) == 'number' then
		return signal(function(t)
			return math.sin(2 * math.pi * f * t)
		end)
	else
		return signal(function(t)
			return math.sin(2 * math.pi * f(t) * t)
		end)
	end

end

function sine_unit(f)
	return 0.5 * sine_wave(f) + 1
end

function square_wave(f)
	return pwm(.5, f)
end

function pwm(d, f)

	if type(d) == 'number' then
		if type(f) == 'number' then
			return signal(function(t)
				if (t * f) % 1 < d then
					return 1.0
				else
					return -1.0
				end
			end)
		else
			return signal(function(t)
				if (t * f(t)) % 1 < d then
					return 1.0
				else
					return -1.0
				end
			end)
		end
	else
		if type(f) == 'number' then
			return signal(function(t)
				if (t * f) % 1 < d(t) then
					return 1.0
				else
					return -1.0
				end
			end)
		else
			return signal(function(t)
				if (t * f(t)) % 1 < d(t) then
					return 1.0
				else
					return -1.0
				end
			end)
		end
	end

end

function square_unit(f)
	f = tosignal(f)
	return signal(
		function(t)
			if (t * f(t)) % 1 < .5 then
				return 1.0
			else
				return 0.0
			end
		end
	)
end

function step(t)
	return signal(function(t1) return math.floor(t(t1)) end)
end

--
-- Envelope generators
--

function adsr_envelope(at, aa, dt, da, st, sa, rt)
	return signal(
		function(t)

			if (at > 0 and t < at) then
				return (aa / at) * t + 0
			end
			t = t - at

			if (dt > 0 and t < dt) then
				return ((da - aa) / dt) * t + aa
			end
			t = t - dt

			if (st > 0 and t < st) then
				return ((sa - da) / st) * t + da
			end
			t = t - st

			if (rt > 0 and t < rt) then
				return ((0 - sa) / rt) * t + sa
			end

			return 0.0
		end
	)
end

function square_pulse_envelope(duration)
	return signal(
		function(t)
			if t < duration then return 1.0 else return 0.0 end
		end
	)
end

function triangle_pulse_envelope(duration)
	return signal(
		function(t)
			if t < 0 then
				return 0.0
			elseif t < duration then
				return 1 - math.abs(1 - 2 / duration * t)
			else
				return 0.0
			end
		end
	)
end

--
-- Simple effects
--

function bitcrush(bits)
	return signal(
		function(a)
			return (2 ^ -bits) * bit.tobit(a * (2 ^ bits))
		end
	)
end

function munch(b, k, f)
	return signal(
		function(t)
			local x = bit.tobit(t * f(t) * (2 ^ b))
			return math.sqrt(bit.band(x, -x) / (2 ^ b))
		end
	)
end

--
-- Pitches
--

function pitch(note)
	note = tosignal(note)
	return signal(function(t) return 440 * 2 ^ ((note(t) - 49) / 12) end)
end
