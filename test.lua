dofile("synth.lua")

local sound = loop(
	adsr_envelope(0.2, 0.5, 0.45, 0.5, 0.002, 0.9, 0.7) * pwm(.50, 220) % 2.5
) * 0.1

function main_sound(time, angle)
	return sound(time)
end
