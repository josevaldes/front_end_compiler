; ModuleID = 'tmp.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@y = global i32 0

define i32 @test() {
entry:
  %x = alloca i32
  store i32 1, i32* %x
  %z = alloca i32
  store i32 1, i32* %z
  %0 = load i32* @y
  switch i32 %0, label %default [
    i32 1, label %case
  ]

footer:                                           ; preds = %case, %default
  %1 = load i32* %z
  %2 = load i32* %y
  %3 = mul i32 %2, 100
  %4 = load i32* %x1
  %5 = mul i32 %4, 10000
  %6 = add i32 %5, %3
  %7 = add i32 %6, %1
  ret i32 %7

default:                                          ; preds = %entry
  br label %footer

case:                                             ; preds = %entry
  %y = alloca i32
  store i32 5, i32* %y
  %x1 = alloca i32
  store i32 8, i32* %x1
  %8 = load i32* %x1
  %9 = load i32* %y
  %10 = add i32 %9, %8
  store i32 %10, i32* %z
  br label %footer
}
