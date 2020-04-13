#ifndef PATCHMATCHER_H
#define PATCHMATCHER_H

#include "Configuration/Configuration.h"

class NNF;

/**
 * @brief The PatchMatcher class This is the interface for the
 * implementation-specific PatchMatch function, which is used in the creation of
 * NNFs (nearest-neighbor fields).
 */
class PatchMatcher {
public:
  PatchMatcher() = default;
  virtual ~PatchMatcher() = default;

  /**
   * @brief patchMatch This is a wrapper around implementationOfPatchMatch. It
   * currently doesn't do any error checks, but I included it so that
   * PatchMatcher's format is the same as that of Downscaler, NNFUpscaler, etc.
   * @param configuration the configuration StyLit is running
   * @param nnf the NNF that should be improved with PatchMatch
   * @param blacklist Another NNF of pixels that should not be mapped to. See
   * the comment for implementationOfPatchMatch for more details.
   * @return true if patch matching succeeds; otherwise false
   */
  bool patchMatch(const Configuration &configuration, NNF &nnf,
                  const NNF *const blacklist = nullptr);

protected:
  /**
   * @brief implementationOfPatchMatch Runs PatchMatch to improve the specified
   * NNF. If a blacklist is specified, coordinates that map to valid coordinates
   * in blacklist should not be mapped to.
   * @param configuration the configuration StyLit is running
   * @param nnf the NNF that should be improved with PatchMatch
   * @param blacklist This optional NNF goes in the opposite direction of NNF.
   * Source coordinates that correspond to valid mappings in blacklist should
   * not be mapped to. This is used in the iterative creation of NNFs via
   * reverse NNFs in the knee point step, where parts of the forward NNF that
   * have already been mapped shouldn't be mapped again.
   * @return true if patch matching succeeds; otherwise false
   */
  virtual bool
  implementationOfPatchMatch(const Configuration &configuration, NNF &nnf,
                             const NNF *const blacklist = nullptr) = 0;
};

#endif // PATCHMATCHER_H