TapeCh : UGen {
	*ar { arg in = 0.0, bias = 0.5, saturation = 0.5, drive = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, bias, saturation, drive).madd(mul, add)
	}
}
