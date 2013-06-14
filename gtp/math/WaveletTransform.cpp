#include "WaveletTransform.h"
#include "../geometry/CubeMap.h"

const real Wavelet::d4normEvens = ( √3 - 1 ) / √2 ; // 0.518 // normalization factor for the evens
const real Wavelet::d4normOdds = ( √3 + 1 ) / √2 ;  // 1.931 // normalization factor for the odds
  
// note ( √3 + 1 ) / √2 == 1/(( √3 - 1 ) / √2)
const real Wavelet::d4unNormEvens = ( √3 + 1 ) / √2 ; // 1.9318516525781365734994863994578    // UN-normalization factor for the evens
const real Wavelet::d4unNormOdds = ( √3 - 1 ) / √2 ;  // 0.5176380902050415246977976752481    // UN-normalization factor for the odds

const real Wavelet::d4c1 = √3 / 4.0 ;
const real Wavelet::d4c2 = (√3 - 2) / 4.0 ;
