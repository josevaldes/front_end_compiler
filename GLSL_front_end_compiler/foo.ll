; ModuleID = 'foo.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@v = global <4 x float> zeroinitializer

define float @foo() {
entry:
  %0 = alloca <4 x float>
  %1 = load <4 x float>* %0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 0
  %3 = insertelement <4 x float> %2, float 1.000000e+00, i32 1
  %4 = insertelement <4 x float> %3, float 1.000000e+00, i32 2
  %5 = insertelement <4 x float> %4, float 1.000000e+00, i32 3
  %6 = load <4 x float>* @v
  %7 = fadd <4 x float> %5, %6
  store <4 x float> %7, <4 x float>* @v
  %8 = load <4 x float>* @v
  %9 = extractelement <4 x float> %8, i32 0
  ret float %9
}
