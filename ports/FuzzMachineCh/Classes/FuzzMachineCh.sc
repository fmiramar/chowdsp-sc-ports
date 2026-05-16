FuzzMachineCh : UGen {
	*ar { arg in = 0.0, fuzz = 0.5, model = 2, volume = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, fuzz, model, volume).madd(mul, add)
	}
}
