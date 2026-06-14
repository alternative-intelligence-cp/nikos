target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

@p = common global [2 x [2 x [2 x i32]]] zeroinitializer, align 16

define void @kalman_global() {
; CHECK-LABEL: define void @kalman_global() {
bb_1:
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 0, i64 0, i64 0), align 16
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 0, i64 0, i64 1), align 4
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 0, i64 1, i64 0), align 8
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 0, i64 1, i64 1), align 4
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 1, i64 0, i64 0), align 16
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 1, i64 0, i64 1), align 4
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 1, i64 1, i64 0), align 8
  store i32 1, i32* getelementptr inbounds ([2 x [2 x [2 x i32]]], [2 x [2 x [2 x i32]]]* @p, i64 0, i64 1, i64 1, i64 1), align 4
  ret void
}

define i32 @main(i32 %arg_1, i8** %arg_2) {
bb_1:
  %_1 = alloca i32, align 4
  %_2 = alloca i32, align 4
  %_3 = alloca i8**, align 8
  store i32 0, i32* %_1, align 4
  store i32 %arg_1, i32* %_2, align 4
  store i8** %arg_2, i8*** %_3, align 8
  call void @kalman_global()
  ret i32 0
}
