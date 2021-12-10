filesize fractal.sk: 189 kb, numOfBlocks used is 13165
         bands.sk: 240 bytes

The program finds blocks in order of colour, overlapping colours over each other from most frequent to least frequent colour.
It then converts it from PGM struct to rectangleArr struct to byteArr to file. This uses a system of imageMasks to hold the pixels that have been set
and the pixels that can be overwritten without loss of data.