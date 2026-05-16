BlondeDriveCh : UGen {
	*ar { arg in = 0.0, drive = 0.5, character = 0.0, bias = 0.5, highQuality = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, drive, character, bias, highQuality).madd(mul, add)
	}
}
