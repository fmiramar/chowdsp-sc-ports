SallenKeyCh : UGen {
	*ar { arg in = 0.0, freq = 2000.0, q = 0.7071, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, q).madd(mul, add)
	}
}
