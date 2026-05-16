SimpleReverbCh : MultiOutUGen {
	*ar { arg inL = 0.0, inR = 0.0, diffusionTime = 100.0, fdnDelay = 100.0, fdnT60Low = 500.0, fdnT60High = 500.0, modAmount = 0.0, dryWet = 0.25;
		^this.multiNew('audio', 2, inL, inR, diffusionTime, fdnDelay, fdnT60Low, fdnT60High, modAmount, dryWet)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}
