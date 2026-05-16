BassFaceCh : UGen {
	*ar { arg in = 0.0, gain = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, gain).madd(mul, add)
	}
}
