#include "NNFUpscalerCPU.h"

#include "Algorithm/NNF.h"

#include <QtGlobal>

bool NNFUpscalerCPU::upscaleNNFCPU(const Configuration &configuration,
                             const NNF &half, NNF &full) {
  assert(half.sourceDimensions.halfTheSizeOf(full.sourceDimensions) &&
         half.targetDimensions.halfTheSizeOf(full.targetDimensions));
  return implementationOfUpscaleNNFCPU(configuration, half, full);
}

bool NNFUpscalerCPU::implementationOfUpscaleNNFCPU(const Configuration &configuration,
                                const NNF &half, NNF &full) {


    for (int i = 0; i < full.targetDimensions.rows; i++) {
        for (int j = 0; j < full.targetDimensions.cols; j++) {
            ImageCoordinates temp = half.getMapping( {qBound(0, i/2, half.targetDimensions.rows - 1),
                                                      qBound(0, j/2, half.targetDimensions.cols - 1)} ) * 2
                                                       + ImageCoordinates{i % 2, j % 2};

            // 5 is the patch size
            full.setMapping({i,j}, {qBound(5, temp.row, full.sourceDimensions.rows - 5 - 1),
                            qBound(5, temp.col, full.sourceDimensions.cols - 5 - 1)});
        }
    }

    return 1;
}
