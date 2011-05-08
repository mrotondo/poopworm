{|pitch| 
	var env = XLine.kr(1, 0.0001, 0.03);
	var sig = BPF.ar(WhiteNoise.ar(1000) * env, pitch, 0.3);
	CombN.ar(sig, 0.01, XLine.kr(0.01, 0.001, 20), -0.2);
};