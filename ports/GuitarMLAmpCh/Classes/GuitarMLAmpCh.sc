GuitarMLAmpCh : UGen {
	*ar { arg in = 0.0, model = 0, condition = 0.5, sampleRateCorrection = 1.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, model, condition, sampleRateCorrection).madd(mul, add)
	}
}
