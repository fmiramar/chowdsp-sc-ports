PulseShaper808Ch : UGen {
	*ar { arg trig = 0.0, width = 0.5, decay = 0.5, doubleTap = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', trig, width, decay, doubleTap).madd(mul, add)
	}
}
