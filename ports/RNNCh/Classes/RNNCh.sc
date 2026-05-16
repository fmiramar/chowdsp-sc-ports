RNNCh : UGen {
	*ar { arg in1 = 0.0, in2 = 0.0, in3 = 0.0, in4 = 0.0, randomize = 0.0, seed = 0.0, reset = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in1, in2, in3, in4, randomize, seed, reset).madd(mul, add)
	}
}
