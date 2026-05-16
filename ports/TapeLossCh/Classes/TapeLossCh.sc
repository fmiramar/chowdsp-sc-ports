TapeLossCh : UGen {
	*ar { arg in = 0.0, gap = 0.5, thickness = 0.5, spacing = 0.5, speed = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, gap, thickness, spacing, speed).madd(mul, add)
	}
}
