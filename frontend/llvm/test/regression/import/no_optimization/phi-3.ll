; ModuleID = 'phi-3.pp.bc'
source_filename = "phi-3.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.14.0"

; CHECK-LABEL: // Bundle
; CHECK: target-endianness = little-endian
; CHECK: target-pointer-size = 64 bits
; CHECK: target-triple = x86_64-apple-macosx10.14.0

declare void @__ikos_assert(i32) #2

; Function Attrs: noinline nounwind ssp uwtable
define i32 @foo(i32*, i32*, i32*) #0 !dbg !8 {
  %4 = alloca i32*, align 8
  %5 = alloca i32*, align 8
  %6 = alloca i32*, align 8
  %7 = alloca i32*, align 8
  %8 = alloca i32*, align 8
  %9 = alloca i32, align 4
  store i32* %0, i32** %4, align 8
  call void @llvm.dbg.declare(metadata i32** %4, metadata !13, metadata !DIExpression()), !dbg !14
  store i32* %1, i32** %5, align 8
  call void @llvm.dbg.declare(metadata i32** %5, metadata !15, metadata !DIExpression()), !dbg !16
  store i32* %2, i32** %6, align 8
  call void @llvm.dbg.declare(metadata i32** %6, metadata !17, metadata !DIExpression()), !dbg !18
  call void @llvm.dbg.declare(metadata i32** %7, metadata !19, metadata !DIExpression()), !dbg !20
  call void @llvm.dbg.declare(metadata i32** %8, metadata !21, metadata !DIExpression()), !dbg !22
  %10 = load i32*, i32** %4, align 8, !dbg !23
  %11 = getelementptr inbounds i32, i32* %10, i64 1, !dbg !24
  store i32* %11, i32** %7, align 8, !dbg !25
  %12 = load i32*, i32** %5, align 8, !dbg !26
  %13 = getelementptr inbounds i32, i32* %12, i64 2, !dbg !27
  store i32* %13, i32** %8, align 8, !dbg !28
  %14 = load i32*, i32** %7, align 8, !dbg !29
  %15 = load i32*, i32** %8, align 8, !dbg !31
  %16 = icmp eq i32* %14, %15, !dbg !32
  br i1 %16, label %17, label %22, !dbg !33

17:                                               ; preds = %3
  %18 = load i32*, i32** %8, align 8, !dbg !34
  %19 = getelementptr inbounds i32, i32* %18, i64 -10, !dbg !36
  store i32* %19, i32** %8, align 8, !dbg !37
  %20 = load i32*, i32** %7, align 8, !dbg !38
  %21 = getelementptr inbounds i32, i32* %20, i64 42, !dbg !39
  store i32* %21, i32** %7, align 8, !dbg !40
  br label %22, !dbg !41

22:                                               ; preds = %17, %3
  %23 = load i32*, i32** %7, align 8, !dbg !42
  %24 = load i32, i32* %23, align 4, !dbg !43
  %25 = icmp eq i32 %24, 3, !dbg !44
  %26 = zext i1 %25 to i32, !dbg !44
  call void @__ikos_assert(i32 %26), !dbg !45
  %27 = load i32*, i32** %8, align 8, !dbg !46
  %28 = load i32, i32* %27, align 4, !dbg !47
  %29 = icmp eq i32 %28, 6, !dbg !48
  %30 = zext i1 %29 to i32, !dbg !48
  call void @__ikos_assert(i32 %30), !dbg !49
  call void @llvm.dbg.declare(metadata i32* %9, metadata !50, metadata !DIExpression()), !dbg !51
  %31 = load i32*, i32** %6, align 8, !dbg !52
  %32 = load i32*, i32** %7, align 8, !dbg !53
  %33 = load i32, i32* %32, align 4, !dbg !54
  %34 = load i32*, i32** %8, align 8, !dbg !55
  %35 = load i32, i32* %34, align 4, !dbg !56
  %36 = add nsw i32 %33, %35, !dbg !57
  %37 = sext i32 %36 to i64, !dbg !52
  %38 = getelementptr inbounds i32, i32* %31, i64 %37, !dbg !52
  %39 = load i32, i32* %38, align 4, !dbg !52
  store i32 %39, i32* %9, align 4, !dbg !51
  %40 = load i32*, i32** %6, align 8, !dbg !58
  %41 = load i32*, i32** %7, align 8, !dbg !59
  %42 = load i32, i32* %41, align 4, !dbg !60
  %43 = load i32*, i32** %8, align 8, !dbg !61
  %44 = load i32, i32* %43, align 4, !dbg !62
  %45 = add nsw i32 %42, %44, !dbg !63
  %46 = sext i32 %45 to i64, !dbg !58
  %47 = getelementptr inbounds i32, i32* %40, i64 %46, !dbg !58
  store i32 555, i32* %47, align 4, !dbg !64
  %48 = load i32, i32* %9, align 4, !dbg !65
  ret i32 %48, !dbg !66
}
; CHECK: declare void @ar.ikos.assert(ui32)
; CHECK: define si32 @foo(opaque* %1, opaque* %2, opaque* %3) {
; CHECK: #1 !entry successors={#2, #3} {
; CHECK:   opaque** $4 = allocate opaque*, 1, align 8
; CHECK:   opaque** $5 = allocate opaque*, 1, align 8
; CHECK:   opaque** $6 = allocate opaque*, 1, align 8
; CHECK:   opaque** $7 = allocate opaque*, 1, align 8
; CHECK:   opaque** $8 = allocate opaque*, 1, align 8
; CHECK:   si32* $9 = allocate si32, 1, align 4
; CHECK:   opaque* %10 = bitcast %1
; CHECK:   opaque** %11 = bitcast $4
; CHECK:   store %11, %10, align 8
; CHECK:   opaque* %12 = bitcast %2
; CHECK:   opaque** %13 = bitcast $5
; CHECK:   store %13, %12, align 8
; CHECK:   opaque* %14 = bitcast %3
; CHECK:   opaque** %15 = bitcast $6
; CHECK:   store %15, %14, align 8
; CHECK:   opaque** %16 = bitcast $4
; CHECK:   opaque* %17 = load %16, align 8
; CHECK:   si32* %18 = bitcast %17
; CHECK:   opaque* %19 = ptrshift %18, 4 * 1
; CHECK:   opaque* %20 = bitcast %19
; CHECK:   opaque** %21 = bitcast $7
; CHECK:   store %21, %20, align 8
; CHECK:   opaque** %22 = bitcast $5
; CHECK:   opaque* %23 = load %22, align 8
; CHECK:   si32* %24 = bitcast %23
; CHECK:   opaque* %25 = ptrshift %24, 4 * 2
; CHECK:   opaque* %26 = bitcast %25
; CHECK:   opaque** %27 = bitcast $8
; CHECK:   store %27, %26, align 8
; CHECK:   opaque** %28 = bitcast $7
; CHECK:   opaque* %29 = load %28, align 8
; CHECK:   opaque** %30 = bitcast $8
; CHECK:   opaque* %31 = load %30, align 8
; CHECK: }
; CHECK: #2 predecessors={#1} successors={#4} {
; CHECK:   %29 peq %31
; CHECK:   opaque** %32 = bitcast $8
; CHECK:   opaque* %33 = load %32, align 8
; CHECK:   si32* %34 = bitcast %33
; CHECK:   opaque* %35 = ptrshift %34, 4 * -10
; CHECK:   opaque* %36 = bitcast %35
; CHECK:   opaque** %37 = bitcast $8
; CHECK:   store %37, %36, align 8
; CHECK:   opaque** %38 = bitcast $7
; CHECK:   opaque* %39 = load %38, align 8
; CHECK:   si32* %40 = bitcast %39
; CHECK:   opaque* %41 = ptrshift %40, 4 * 42
; CHECK:   opaque* %42 = bitcast %41
; CHECK:   opaque** %43 = bitcast $7
; CHECK:   store %43, %42, align 8
; CHECK: }
; CHECK: #3 predecessors={#1} successors={#4} {
; CHECK:   %29 pne %31
; CHECK: }
; CHECK: #4 predecessors={#3, #2} successors={#5, #6} {
; CHECK:   opaque** %44 = bitcast $7
; CHECK:   opaque* %45 = load %44, align 8
; CHECK:   si32* %46 = bitcast %45
; CHECK:   si32 %47 = load %46, align 4
; CHECK:   void (si32)* %48 = bitcast @ar.ikos.assert
; CHECK:   void (si32)* %49 = bitcast @ar.ikos.assert
; CHECK: }
; CHECK: #5 predecessors={#4} successors={#7} {
; CHECK:   %47 sieq 3
; CHECK:   ui1 %50 = 1
; CHECK: }
; CHECK: #6 predecessors={#4} successors={#7} {
; CHECK:   %47 sine 3
; CHECK:   ui1 %50 = 0
; CHECK: }
; CHECK: #7 predecessors={#5, #6} successors={#8, #9} {
; CHECK:   ui32 %51 = zext %50
; CHECK:   si32 %52 = bitcast %51
; CHECK:   call %48(%52)
; CHECK:   opaque** %53 = bitcast $8
; CHECK:   opaque* %54 = load %53, align 8
; CHECK:   si32* %55 = bitcast %54
; CHECK:   si32 %56 = load %55, align 4
; CHECK: }
; CHECK: #8 predecessors={#7} successors={#10} {

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind ssp uwtable
define i32 @main(i32, i8**) #0 !dbg !67 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca [2 x i32], align 4
  %7 = alloca [3 x i32], align 4
  %8 = alloca [10 x i32], align 16
  %9 = alloca i32, align 4
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  call void @llvm.dbg.declare(metadata i32* %4, metadata !73, metadata !DIExpression()), !dbg !74
  store i8** %1, i8*** %5, align 8
  call void @llvm.dbg.declare(metadata i8*** %5, metadata !75, metadata !DIExpression()), !dbg !76
  call void @llvm.dbg.declare(metadata [2 x i32]* %6, metadata !77, metadata !DIExpression()), !dbg !81
  call void @llvm.dbg.declare(metadata [3 x i32]* %7, metadata !82, metadata !DIExpression()), !dbg !86
  call void @llvm.dbg.declare(metadata [10 x i32]* %8, metadata !87, metadata !DIExpression()), !dbg !91
  %10 = getelementptr inbounds [10 x i32], [10 x i32]* %8, i64 0, i64 9, !dbg !92
  store i32 666, i32* %10, align 4, !dbg !93
  %11 = getelementptr inbounds [2 x i32], [2 x i32]* %6, i64 0, i64 0, !dbg !94
  store i32 1, i32* %11, align 4, !dbg !95
  %12 = getelementptr inbounds [2 x i32], [2 x i32]* %6, i64 0, i64 1, !dbg !96
  store i32 3, i32* %12, align 4, !dbg !97
  %13 = getelementptr inbounds [3 x i32], [3 x i32]* %7, i64 0, i64 0, !dbg !98
  store i32 4, i32* %13, align 4, !dbg !99
  %14 = getelementptr inbounds [3 x i32], [3 x i32]* %7, i64 0, i64 1, !dbg !100
  store i32 5, i32* %14, align 4, !dbg !101
  %15 = getelementptr inbounds [3 x i32], [3 x i32]* %7, i64 0, i64 2, !dbg !102
  store i32 6, i32* %15, align 4, !dbg !103
  call void @llvm.dbg.declare(metadata i32* %9, metadata !104, metadata !DIExpression()), !dbg !105
  %16 = getelementptr inbounds [2 x i32], [2 x i32]* %6, i64 0, i64 0, !dbg !106
  %17 = getelementptr inbounds [3 x i32], [3 x i32]* %7, i64 0, i64 0, !dbg !107
  %18 = getelementptr inbounds [10 x i32], [10 x i32]* %8, i64 0, i64 0, !dbg !108
  %19 = call i32 @foo(i32* %16, i32* %17, i32* %18), !dbg !109
  store i32 %19, i32* %9, align 4, !dbg !105
  %20 = load i32, i32* %9, align 4, !dbg !110
  %21 = icmp eq i32 %20, 666, !dbg !111
  %22 = zext i1 %21 to i32, !dbg !111
  call void @__ikos_assert(i32 %22), !dbg !112
  %23 = getelementptr inbounds [10 x i32], [10 x i32]* %8, i64 0, i64 9, !dbg !113
  %24 = load i32, i32* %23, align 4, !dbg !113
  %25 = icmp eq i32 %24, 555, !dbg !114
  %26 = zext i1 %25 to i32, !dbg !114
  call void @__ikos_assert(i32 %26), !dbg !115
  %27 = load i32, i32* %9, align 4, !dbg !116
  ret i32 %27, !dbg !117
}
; CHECK:   %56 sieq 6
; CHECK:   ui1 %57 = 1
; CHECK: }
; CHECK: #9 predecessors={#7} successors={#10} {
; CHECK:   %56 sine 6
; CHECK:   ui1 %57 = 0
; CHECK: }
; CHECK: #10 !exit predecessors={#8, #9} {
; CHECK:   ui32 %58 = zext %57
; CHECK:   si32 %59 = bitcast %58
; CHECK:   call %49(%59)
; CHECK:   opaque** %60 = bitcast $6
; CHECK:   opaque* %61 = load %60, align 8
; CHECK:   opaque** %62 = bitcast $7
; CHECK:   opaque* %63 = load %62, align 8
; CHECK:   si32* %64 = bitcast %63
; CHECK:   si32 %65 = load %64, align 4
; CHECK:   opaque** %66 = bitcast $8
; CHECK:   opaque* %67 = load %66, align 8
; CHECK:   si32* %68 = bitcast %67
; CHECK:   si32 %69 = load %68, align 4
; CHECK:   si32 %70 = %65 sadd.nw %69
; CHECK:   si64 %71 = sext %70
; CHECK:   si32* %72 = bitcast %61
; CHECK:   opaque* %73 = ptrshift %72, 4 * %71
; CHECK:   si32* %74 = bitcast %73
; CHECK:   si32 %75 = load %74, align 4
; CHECK:   store $9, %75, align 4
; CHECK:   opaque** %76 = bitcast $6
; CHECK:   opaque* %77 = load %76, align 8
; CHECK:   opaque** %78 = bitcast $7
; CHECK:   opaque* %79 = load %78, align 8
; CHECK:   si32* %80 = bitcast %79
; CHECK:   si32 %81 = load %80, align 4
; CHECK:   opaque** %82 = bitcast $8
; CHECK:   opaque* %83 = load %82, align 8
; CHECK:   si32* %84 = bitcast %83
; CHECK:   si32 %85 = load %84, align 4
; CHECK:   si32 %86 = %81 sadd.nw %85
; CHECK:   si64 %87 = sext %86
; CHECK:   si32* %88 = bitcast %77
; CHECK:   opaque* %89 = ptrshift %88, 4 * %87
; CHECK:   si32* %90 = bitcast %89
; CHECK:   store %90, 555, align 4
; CHECK:   si32 %91 = load $9, align 4
; CHECK:   return %91
; CHECK: }
; CHECK: }
; CHECK: define si32 @main(si32 %1, opaque* %2) {
; CHECK: #1 !entry successors={#2, #3} {
; CHECK:   opaque* $3 = allocate opaque, 1, align 4
; CHECK:   si32* $4 = allocate si32, 1, align 4
; CHECK:   opaque** $5 = allocate opaque*, 1, align 8
; CHECK:   [2 x si32]* $6 = allocate [2 x si32], 1, align 4
; CHECK:   [3 x si32]* $7 = allocate [3 x si32], 1, align 4
; CHECK:   [10 x si32]* $8 = allocate [10 x si32], 1, align 16
; CHECK:   si32* $9 = allocate si32, 1, align 4
; CHECK:   si32* %10 = bitcast $3
; CHECK:   store %10, 0, align 4

attributes #0 = { noinline nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 9.0.0 (tags/RELEASE_900/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, nameTableKind: GNU)
!1 = !DIFile(filename: "phi-3.c", directory: "/Users/marthaud/ikos/ikos-git/frontend/llvm/test/regression/import/no_optimization")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{i32 7, !"PIC Level", i32 2}
!7 = !{!"clang version 9.0.0 (tags/RELEASE_900/final)"}
!8 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 5, type: !9, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!9 = !DISubroutineType(types: !10)
!10 = !{!11, !12, !12, !12}
!11 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!12 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !11, size: 64)
!13 = !DILocalVariable(name: "a", arg: 1, scope: !8, file: !1, line: 5, type: !12)
!14 = !DILocation(line: 5, column: 14, scope: !8)
!15 = !DILocalVariable(name: "b", arg: 2, scope: !8, file: !1, line: 5, type: !12)
!16 = !DILocation(line: 5, column: 22, scope: !8)
!17 = !DILocalVariable(name: "c", arg: 3, scope: !8, file: !1, line: 5, type: !12)
!18 = !DILocation(line: 5, column: 30, scope: !8)
!19 = !DILocalVariable(name: "p", scope: !8, file: !1, line: 6, type: !12)
!20 = !DILocation(line: 6, column: 8, scope: !8)
!21 = !DILocalVariable(name: "q", scope: !8, file: !1, line: 7, type: !12)
!22 = !DILocation(line: 7, column: 8, scope: !8)
!23 = !DILocation(line: 9, column: 7, scope: !8)
!24 = !DILocation(line: 9, column: 9, scope: !8)
!25 = !DILocation(line: 9, column: 5, scope: !8)
!26 = !DILocation(line: 10, column: 7, scope: !8)
!27 = !DILocation(line: 10, column: 9, scope: !8)
!28 = !DILocation(line: 10, column: 5, scope: !8)
!29 = !DILocation(line: 12, column: 7, scope: !30)
!30 = distinct !DILexicalBlock(scope: !8, file: !1, line: 12, column: 7)
!31 = !DILocation(line: 12, column: 12, scope: !30)
!32 = !DILocation(line: 12, column: 9, scope: !30)
!33 = !DILocation(line: 12, column: 7, scope: !8)
!34 = !DILocation(line: 13, column: 9, scope: !35)
!35 = distinct !DILexicalBlock(scope: !30, file: !1, line: 12, column: 15)
!36 = !DILocation(line: 13, column: 11, scope: !35)
!37 = !DILocation(line: 13, column: 7, scope: !35)
!38 = !DILocation(line: 14, column: 9, scope: !35)
!39 = !DILocation(line: 14, column: 11, scope: !35)
!40 = !DILocation(line: 14, column: 7, scope: !35)
!41 = !DILocation(line: 15, column: 3, scope: !35)
!42 = !DILocation(line: 16, column: 18, scope: !8)
!43 = !DILocation(line: 16, column: 17, scope: !8)
!44 = !DILocation(line: 16, column: 20, scope: !8)
!45 = !DILocation(line: 16, column: 3, scope: !8)
!46 = !DILocation(line: 17, column: 18, scope: !8)
!47 = !DILocation(line: 17, column: 17, scope: !8)
!48 = !DILocation(line: 17, column: 20, scope: !8)
!49 = !DILocation(line: 17, column: 3, scope: !8)
!50 = !DILocalVariable(name: "res", scope: !8, file: !1, line: 19, type: !11)
!51 = !DILocation(line: 19, column: 7, scope: !8)
!52 = !DILocation(line: 19, column: 13, scope: !8)
!53 = !DILocation(line: 19, column: 16, scope: !8)
!54 = !DILocation(line: 19, column: 15, scope: !8)
!55 = !DILocation(line: 19, column: 21, scope: !8)
!56 = !DILocation(line: 19, column: 20, scope: !8)
!57 = !DILocation(line: 19, column: 18, scope: !8)
!58 = !DILocation(line: 20, column: 3, scope: !8)
!59 = !DILocation(line: 20, column: 6, scope: !8)
!60 = !DILocation(line: 20, column: 5, scope: !8)
!61 = !DILocation(line: 20, column: 11, scope: !8)
!62 = !DILocation(line: 20, column: 10, scope: !8)
!63 = !DILocation(line: 20, column: 8, scope: !8)
!64 = !DILocation(line: 20, column: 14, scope: !8)
!65 = !DILocation(line: 21, column: 10, scope: !8)
!66 = !DILocation(line: 21, column: 3, scope: !8)
!67 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 24, type: !68, scopeLine: 24, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!68 = !DISubroutineType(types: !69)
!69 = !{!11, !11, !70}
!70 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !71, size: 64)
!71 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !72, size: 64)
!72 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!73 = !DILocalVariable(name: "argc", arg: 1, scope: !67, file: !1, line: 24, type: !11)
!74 = !DILocation(line: 24, column: 14, scope: !67)
!75 = !DILocalVariable(name: "argv", arg: 2, scope: !67, file: !1, line: 24, type: !70)
!76 = !DILocation(line: 24, column: 27, scope: !67)
!77 = !DILocalVariable(name: "a", scope: !67, file: !1, line: 25, type: !78)
!78 = !DICompositeType(tag: DW_TAG_array_type, baseType: !11, size: 64, elements: !79)
!79 = !{!80}
!80 = !DISubrange(count: 2)
!81 = !DILocation(line: 25, column: 7, scope: !67)
!82 = !DILocalVariable(name: "b", scope: !67, file: !1, line: 26, type: !83)
!83 = !DICompositeType(tag: DW_TAG_array_type, baseType: !11, size: 96, elements: !84)
!84 = !{!85}
!85 = !DISubrange(count: 3)
!86 = !DILocation(line: 26, column: 7, scope: !67)
!87 = !DILocalVariable(name: "c", scope: !67, file: !1, line: 27, type: !88)
!88 = !DICompositeType(tag: DW_TAG_array_type, baseType: !11, size: 320, elements: !89)
!89 = !{!90}
!90 = !DISubrange(count: 10)
!91 = !DILocation(line: 27, column: 7, scope: !67)
!92 = !DILocation(line: 29, column: 3, scope: !67)
!93 = !DILocation(line: 29, column: 8, scope: !67)
!94 = !DILocation(line: 31, column: 3, scope: !67)
!95 = !DILocation(line: 31, column: 8, scope: !67)
!96 = !DILocation(line: 32, column: 3, scope: !67)
!97 = !DILocation(line: 32, column: 8, scope: !67)
!98 = !DILocation(line: 34, column: 3, scope: !67)
!99 = !DILocation(line: 34, column: 8, scope: !67)
!100 = !DILocation(line: 35, column: 3, scope: !67)
!101 = !DILocation(line: 35, column: 8, scope: !67)
!102 = !DILocation(line: 36, column: 3, scope: !67)
!103 = !DILocation(line: 36, column: 8, scope: !67)
!104 = !DILocalVariable(name: "x", scope: !67, file: !1, line: 38, type: !11)
!105 = !DILocation(line: 38, column: 7, scope: !67)
!106 = !DILocation(line: 38, column: 15, scope: !67)
!107 = !DILocation(line: 38, column: 18, scope: !67)
!108 = !DILocation(line: 38, column: 21, scope: !67)
!109 = !DILocation(line: 38, column: 11, scope: !67)
!110 = !DILocation(line: 40, column: 17, scope: !67)
!111 = !DILocation(line: 40, column: 19, scope: !67)
!112 = !DILocation(line: 40, column: 3, scope: !67)
!113 = !DILocation(line: 41, column: 17, scope: !67)
!114 = !DILocation(line: 41, column: 22, scope: !67)
!115 = !DILocation(line: 41, column: 3, scope: !67)
!116 = !DILocation(line: 42, column: 10, scope: !67)
!117 = !DILocation(line: 42, column: 3, scope: !67)
