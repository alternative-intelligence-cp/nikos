target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"

%struct.foo = type { i32 }

@x = global i32 6, align 4
@B = common global [10 x i32] zeroinitializer, align 16

define i32 @main(i32 %arg_1, i8** %arg_2) {
; CHECK-LABEL: define i32 @main(i32 %arg_1, ptr %arg_2) {
bb_1:
  %_1 = alloca i32, align 4
  %_2 = alloca i32, align 4
  %_3 = alloca i8**, align 8
  %_4 = alloca [5 x i32], align 16
  %_5 = alloca %struct.foo, align 4
  %_6 = alloca i32, align 4
  store i32 0, i32* %_1, align 4
  store i32 %arg_1, i32* %_2, align 4
  store i8** %arg_2, i8*** %_3, align 8
  %_7 = bitcast [5 x i32]* %_4 to i8*
  call void @llvm.memset.p0.i64(i8* align 16 %_7, i8 0, i64 20, i1 false)
  %_8 = getelementptr inbounds %struct.foo, %struct.foo* %_5, i32 0, i32 0
  store i32 59, i32* %_8, align 4
  %_9 = load i32, i32* @x, align 4
  %_10 = add nsw i32 %_9, 1
  store i32 %_10, i32* @x, align 4
  store i32 23, i32* getelementptr inbounds ([10 x i32], [10 x i32]* @B, i64 0, i64 8), align 16
  %_11 = load i32, i32* @x, align 4
  %_12 = add nsw i32 %_11, 7
  %_13 = getelementptr inbounds %struct.foo, %struct.foo* %_5, i32 0, i32 0
  %_14 = load i32, i32* %_13, align 4
  %_15 = add nsw i32 %_12, %_14
  %_16 = getelementptr inbounds [5 x i32], [5 x i32]* %_4, i64 0, i64 4
  %_17 = load i32, i32* %_16, align 16
  %_18 = add nsw i32 %_15, %_17
  %_19 = load i32, i32* getelementptr inbounds ([10 x i32], [10 x i32]* @B, i64 0, i64 9), align 4
  %_20 = add nsw i32 %_18, %_19
  store i32 %_20, i32* %_6, align 4
  %_21 = load i32, i32* %_6, align 4
  ret i32 %_21
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0.i64(i8* nocapture writeonly, i8, i64, i1) #0

attributes #0 = { argmemonly nounwind }
