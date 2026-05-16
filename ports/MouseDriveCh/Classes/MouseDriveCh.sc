MouseDriveCh : UGen {
	*ar { arg in = 0.0, distortion = 0.75, volume = 0.75, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, distortion, volume).madd(mul, add)
	}
}
