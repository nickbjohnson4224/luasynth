dofile("synth.lua")

local sound = loop(
	adsr_envelope(0.02, 0.5, 0.15, 0.0, 0.2, 0.0, 0.1) * pwm(.50, 220) % 1.5
) * 0.1

function main_sound(time, angle)
	return sound(time)
end
