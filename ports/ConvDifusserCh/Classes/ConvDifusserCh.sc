ConvDifusserCh : MultiOutUGen {
	*ar { arg inL = 0.0, inR = 0.0, diffusionTime = 100.0;
		^this.multiNew('audio', 2, inL, inR, diffusionTime)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}
