{|input|
	var lfo;
	var out = input;
	lfo = SinOsc.kr(0.2, [0,0.5pi], 0.0024, 0.0025);
	3.do { out = DelayL.ar(out, 0.1, lfo, 1, out) };
	out;
}