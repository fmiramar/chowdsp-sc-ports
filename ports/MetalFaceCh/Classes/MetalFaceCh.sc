MetalFaceCh : UGen {
	*ar { arg in = 0.0, gainDB = -12.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, gainDB).madd(mul, add)
	}
}
