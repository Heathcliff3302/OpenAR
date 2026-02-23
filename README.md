
//pt3.3为最终版本，
//"pt3.3" will be the latest and final version

//中文
项目目标

本项目旨在构建一个 面向可穿戴 AR 设备的多模态实时交互原型系统，通过整合视觉、语音与大模型推理能力，在真实环境中为用户提供基于空间语义理解的增强现实辅助信息。
系统的核心目标不是单一的 AI 问答或图像识别，而是实现一条完整、可验证的工程闭环：
真实世界感知 → 多模态信息融合 → AI 语义理解与推理 → 虚拟信息层构建 → 实时反馈给用户设备
最终形式为一种 AR-like 的无窗口信息叠加显示，用于辅助用户在现实场景中完成具体任务。

当前项目已完成一个 端到端的多模态采集、推理与显示闭环原型，主要包括：

1. 多模态数据采集（设备侧）

视觉输入
使用 V4L2 接口直接调用摄像头
以 0.5s 间隔采集图像，持续 10 秒
图像格式转换流程：Raw Frame → JPEG → Base64
语音输入
使用 ALSA 进行音频采集（10 秒）
采用 Whisper 小模型完成本地语音转文本

2. 多模态数据封装与传输

将 Base64 编码的图像序列 与 语音转写文本
封装为统一的 JSON 数据结构
通过 HTTP 协议发送至云端 AI 接口
（当前使用：Qwen3-VL-32B-Thinking）

3. AI 推理与结果返回

云端模型基于图像与文本进行多模态理解与推理
返回结构化/非结构化文本响应

4. 无窗口 AR-like 显示输出

通过 DRM / KMS + GBM + EGL 渲染路径
直接在 Framebuffer 上进行文本绘制
不依赖窗口系统（无 X11 / Wayland）
不依赖外部 GUI 或图形库
实现面向嵌入式设备的 低依赖、直接显示输出
该显示方式为后续 AR/MR 虚拟信息层构建提供基础能力。

//English
Project Goal

This project aims to build a multimodal real-time interaction prototype for wearable AR devices.
By integrating visual perception, voice input, and large-scale multimodal model reasoning, the system provides augmented reality assistance based on spatial semantic understanding within real-world environments.

Rather than focusing on standalone AI Q&A or image recognition, the core objective is to implement a complete and verifiable engineering pipeline:

Real-world perception → Multimodal data fusion → AI semantic understanding & reasoning → Virtual information layer construction → Real-time feedback to the user device

The final form is an AR-like, windowless information overlay, designed to assist users in completing real-world tasks.

Current Implementation

The project currently delivers an end-to-end prototype covering multimodal acquisition, inference, and display, including the following components:

1. Multimodal Data Acquisition (Device Side)

Visual Input
Direct camera access via the V4L2 interface
Frame capture at 0.5-second intervals over a 10-second duration
Image processing pipeline:
Raw Frame → JPEG → Base64
Audio Input
Audio capture using ALSA (10-second recording window)
Local speech-to-text conversion using a lightweight Whisper model

2. Multimodal Data Packaging and Transmission

Combination of:
Base64-encoded image sequences
Speech transcription text
Unified packaging into a structured JSON format
Transmission to a cloud-based AI interface via HTTP
(Current model: Qwen3-VL-32B-Thinking)

3. AI Inference and Response Handling

Cloud-side multimodal understanding and reasoning based on visual and textual inputs
Return of structured and/or unstructured textual responses
4. Windowless AR-like Display Output

Rendering pipeline based on DRM / KMS + GBM + EGL
Direct text rendering to the framebuffer
No dependency on window systems (X11 / Wayland)
No reliance on external GUI or graphics libraries
Enables low-dependency, direct display output for embedded devices

This display approach serves as a foundational capability for future AR/MR virtual information layer construction.
