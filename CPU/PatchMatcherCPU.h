#ifndef PATCHMATCHERCPU_H
#define PATCHMATCHERCPU_H

#include "Algorithm/PatchMatcher.h"
#include "Algorithm/FeatureVector.h"
#include "Algorithm/PyramidLevel.h"
#include "Algorithm/ChannelWeights.h"
#include "ErrorCalculatorCPU.h"

int randi(int min, int max) {
  return (std::rand() % (max - min)) + min;
}

class NNF;

/**
 * @brief Implements the PatchMatch algorithm on the CPU
 */

template <typename T, unsigned int numGuideChannels,
          unsigned int numStyleChannels>
class PatchMatcherCPU : public PatchMatcher<T, numGuideChannels, numStyleChannels> {
public:
  PatchMatcherCPU() = default;
  ~PatchMatcherCPU() = default;

private:
  const int NUM_PATCHMATCH_ITERATIONS = 6;

  /**
   * @brief patchMatch This is a wrapper around implementationOfPatchMatch. It
   * currently doesn't do any error checks, but I included it so that
   * PatchMatcher's format is the same as that of Downscaler, NNFUpscaler, etc.
   * @param configuration the configuration StyLit is running
   * @param nnf the NNF that should be improved with PatchMatch, size of domain images,
   *        maps to indices in codomain images
   * @param pyramid the image pyramid
   * @param level the level of the pyramid for which an NNF is being generated
   * @param makeReverseNNF indicates whether a reverse or forward nnf is
   *        being generated
   * @param blacklist Another NNF of pixels that should not be mapped to.
   * @return true if patch matching succeeds; otherwise false
   */
  bool implementationOfPatchMatch(const Configuration &configuration, NNF &nnf,
                                  const Pyramid<T, numGuideChannels, numStyleChannels> &pyramid,
                                  int level, bool makeReverseNNF, bool initRandom, const NNF *const blacklist = nullptr) {
    PyramidLevel<T, numGuideChannels, numStyleChannels> &pyramidLevel = pyramid.levels[level];
    Image<T, numGuideChannels> &domainGuide = makeReverseNNF ? pyramidLevel.guide.source : pyramidLevel.guide.target;
    Image<T, numStyleChannels> &domainStyle = makeReverseNNF ? pyramidLevel.style.source : pyramidLevel.style.target;
    Image<T, numGuideChannels> &codomainGuide = makeReverseNNF ? pyramidLevel.guide.target : pyramidLevel.guide.source;
    Image<T, numStyleChannels> &codomainStyle = makeReverseNNF ? pyramidLevel.style.target : pyramidLevel.style.source;

    int numNNFRows = nnf.sourceDimensions.rows;
    int numNNFCols = nnf.sourceDimensions.cols;

    if (initRandom) {
      randomlyInitializeNNF(nnf);
    }

    for (int i = 0; i < NUM_PATCHMATCH_ITERATIONS; i++) {
      bool iterationIsOdd = i % 2 == 1 ? true : false;
      for (int col = 0; col < numNNFCols; col++) {
        for (int row = 0; row < numNNFRows; row++) {
          propagationStep(configuration, row, col, makeReverseNNF, iterationIsOdd, nnf, level, blacklist);
          searchStep(configuration, row, col, makeReverseNNF, nnf, level, blacklist);
        }
      }
    }
  }

  void propagationStep(const Configuration &configuration, int row, int col, bool makeReverseNNF, bool iterationIsOdd,
                       NNF &nnf, const PyramidLevel<T, numGuideChannels, numStyleChannels> &pyramidLevel,
                       const NNF *const blacklist = nullptr) {
    ErrorCalculatorCPU<T, numGuideChannels, numStyleChannels> errorCalc = ErrorCalculatorCPU<T, numGuideChannels, numStyleChannels>();
    float newPatchError1 = -1.0; // we know if a patch was out of bounds if its error remains -1, so don't consider it the end of this method
    float newPatchError2 = -1.0;
    int offset = iterationIsOdd ? -1 : 1;
    ImageCoordinates currentPatch{row, col};
    ImageCoordinates newPatch1{-1, -1};
    ImageCoordinates newPatch2{-1, -1};
    ImageCoordinates domainNeighbor1{row + offset, col};
    if (nnf.sourceDimensions.within(domainNeighbor1)) {
      ImageCoordinates codomainNeighbor1 = nnf.getMapping(domainNeighbor1);
      // get the patch in the codomain that we might want to map the current (row, col) domain patch to
      // NOTE: the -offset below is from the ebsynth implementation, and is not part of the original patchmatch algorithm
      newPatch1.row = codomainNeighbor1.row - offset;
      newPatch1.col = codomainNeighbor1.col;
      // the blacklist tells us if the codomain index newPatch1 is available.
      // if the corresponding element in the blacklist is (-1,-1), then this new patch is available
      bool newPatch1Available = (blacklist == nullptr) || (blacklist->getMapping(newPatch1).row == -1 &&
                                                           blacklist->getMapping(newPatch1).col == -1);
      if (nnf.targetDimensions.within(newPatch1)) {
        if (newPatch1Available) {
          if (makeReverseNNF) {
            errorCalc.calculateError(configuration, pyramidLevel, currentPatch, newPatch1, newPatchError1);
          } else {
            errorCalc.calculateError(configuration, pyramidLevel, newPatch1, currentPatch, newPatchError1);
          }
        } else {
          newPatchError1 = MAXFLOAT; // if newPatch1 is not available, automatically set the energy to MAXFLOAT
        }
      }
    }

    // do the exact same thing as above but with the analogous col offset
    ImageCoordinates domainNeighbor2{row, col + offset};
    if (nnf.sourceDimensions.within(domainNeighbor2)) {
      ImageCoordinates codomainNeighbor2 = nnf.getMapping(domainNeighbor2);
      // NOTE: we have the same -offset that we have above
      newPatch2.row = codomainNeighbor2.row;
      newPatch2.col = codomainNeighbor2.col - offset;
      bool newPatch2Available = (blacklist == nullptr) || (blacklist->getMapping(newPatch2).row == -1 &&
                                                           blacklist->getMapping(newPatch2).col == -1);
      if (nnf.targetDimensions.within(newPatch2)) {
        if (newPatch2Available) {
          if (makeReverseNNF) {
            errorCalc.calculateError(configuration, pyramidLevel, currentPatch, newPatch2, newPatchError2);
          } else {
            errorCalc.calculateError(configuration, pyramidLevel, newPatch2, currentPatch, newPatchError2);
          }
        } else {
          newPatchError2 = MAXFLOAT;
        }
      }
    }

    // calculate the energy from the current mapping
    float currentError;
    if (makeReverseNNF) {
      errorCalc.calculateError(configuration, pyramidLevel, currentPatch, nnf.getMapping(currentPatch), currentError);
    } else {
      errorCalc.calculateError(configuration, pyramidLevel, nnf.getMapping(currentPatch), currentPatch, currentError);
    }

    // now that we have the errors of the new patches we are considering and the current error, we can decide which one is the best
    bool changedToNewPatch1 = false;
    if (newPatchError1 > 0 && newPatchError1 < currentError) {
      nnf.setMapping(currentPatch, newPatch1);
      changedToNewPatch1 = true;
    }

    if (changedToNewPatch1) {
      if (newPatchError2 > 0 && newPatchError2 < newPatchError1) {
        nnf.setMapping(currentPatch, newPatch2);
      }
    } else {
      if (newPatchError2 > 0 && newPatchError2 < currentError) {
        nnf.setMapping(currentPatch, newPatch2);
      }
    }
  }

  void searchStep(const Configuration &configuration, int row, int col, bool makeReverseNNF, bool iterationIsOdd,
                  NNF &nnf, const PyramidLevel<T, numGuideChannels, numStyleChannels> &pyramidLevel,
                  const NNF *const blacklist = nullptr) {

  }

  void randomlyInitializeNNF(NNF &nnf) {
    for (int col = 0; col < nnf.sourceDimensions.cols; col++) {
      for (int row = 0; row < nnf.sourceDimensions.rows; row++) {
        ImageCoordinates from{row, col};
        ImageCoordinates to{randi(0, nnf.targetDimensions.rows), randi(0, nnf.targetDimensions.col)};
        nnf.setMapping(from, to);
      }
    }
  }

};

#endif // PATCHMATCHERCPU_H
