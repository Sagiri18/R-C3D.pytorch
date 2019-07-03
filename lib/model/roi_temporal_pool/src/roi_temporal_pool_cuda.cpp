#include <torch/extension.h>

#include <cmath>
#include <vector>

int ROIPoolForwardLaucher(const at::Tensor features, const at::Tensor rois,
                          const float temporal_scale, const int channels,
                          const int length, const int height, const int width, const int num_rois,
                          const int pooled_l, const int pooled_h, const int pooled_w,
                          at::Tensor output, at::Tensor argmax);

int ROIPoolBackwardLaucher(const at::Tensor top_grad, const at::Tensor rois,
                           const at::Tensor argmax, const float temporal_scale,
                           const int batch_size, const int channels,
                           const int length, const int height, const int width,
                           const int num_rois, const int pooled_l, const int pooled_h,
                           const int pooled_w, at::Tensor bottom_grad);

#define CHECK_CUDA(x) AT_CHECK(x.type().is_cuda(), #x, " must be a CUDAtensor ")
#define CHECK_CONTIGUOUS(x) \
  AT_CHECK(x.is_contiguous(), #x, " must be contiguous ")
#define CHECK_INPUT(x) \
  CHECK_CUDA(x);       \
  CHECK_CONTIGUOUS(x)

int roi_pooling_forward_cuda(at::Tensor features, at::Tensor rois,
                             int pooled_length, int pooled_height, int pooled_width,
                             float temporal_scale, at::Tensor output,
                             at::Tensor argmax) {
  CHECK_INPUT(features);
  CHECK_INPUT(rois);
  CHECK_INPUT(output);
  CHECK_INPUT(argmax);

  int num_rois = rois.size(0);
  int size_rois = rois.size(1);

  if (size_rois != 3) {
    printf("wrong roi size\n");
    return 0;
  }

  int channels = features.size(1);
  int length = features.size(2);
  int height = features.size(3);
  int width = features.size(4);

  ROIPoolForwardLaucher(features, rois, temporal_scale, channels, length, height, width,
                        num_rois, pooled_length, pooled_height, pooled_width, output, argmax);

  return 1;
}

int roi_pooling_backward_cuda(at::Tensor top_grad, at::Tensor rois,
                              at::Tensor argmax, float temporal_scale,
                              at::Tensor bottom_grad) {
  CHECK_INPUT(top_grad);
  CHECK_INPUT(rois);
  CHECK_INPUT(argmax);
  CHECK_INPUT(bottom_grad);

  int pooled_length = top_grad.size(2);
  int pooled_height = top_grad.size(3);
  int pooled_width = top_grad.size(4);
  int num_rois = rois.size(0);
  int size_rois = rois.size(1);

  if (size_rois != 3) {
    printf("wrong roi size\n");
    return 0;
  }
  int batch_size = bottom_grad.size(0);
  int channels = bottom_grad.size(1);
  int length = bottom_grad.size(2);
  int height = bottom_grad.size(3);
  int width = bottom_grad.size(4);

  ROIPoolBackwardLaucher(top_grad, rois, argmax, temporal_scale, batch_size,
                         channels, length, height, width, num_rois, pooled_length, pooled_height,
                         pooled_width, bottom_grad);

  return 1;
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("forward", &roi_pooling_forward_cuda, "Roi_Pooling forward (CUDA)");
  m.def("backward", &roi_pooling_backward_cuda, "Roi_Pooling backward (CUDA)");
}
