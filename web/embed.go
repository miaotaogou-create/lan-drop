package web

import "embed"

// Dist 是前端构建产物，打进单文件客户端。
//
//go:embed all:dist
var Dist embed.FS
