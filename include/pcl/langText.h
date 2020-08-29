#pragma once

#ifndef PCL_CN

#define PCL_LANG_FILE_NOT_SPECIFIED "warning: image file is not specified"
#define PCL_LANG_FAILED_TO_LOAD     "error: fail to load image file"
#define PCL_LANG_UNMATCHED_SIZE     "error: image size is unmatched"
#define PCL_LANG_SUCCESS_TO_LOAD    "success"

#define PCL_LANG_ADD_LAYER     "add layer"
#define PCL_LANG_LOAD_ALL      "set all papers"
#define PCL_LANG_LOAD_ALL_TIPS "remove all exists papers and recreate from multiple image files"

#define PCL_LANG_RENAME "rename"
#define PCL_LANG_DELETE "delete"
#define PCL_LANG_BROWSE "browse"

#define PCL_LANG_DELETE_WINDOW "delete?"

#define PCL_LANG_OK     "ok"
#define PCL_LANG_CANCEL "cancel"

#define PCL_LANG_NEW_LAYER_NAME "new name"

#define PCL_LANG_BROWSE_LIGHT    "browse light image"
#define PCL_LANG_LIGHT_INTENSITY "light intensity"
#define PCL_LANG_ENV_LIGHT       "environment light"
#define PCL_LANG_LIGHT_DISTANCE  "light distance (mm)"

#define PCL_LANG_PAPER_HORI_SIZE "paper width (mm)"
#define PCL_LANG_PAPER_DISTANCE  "paper distance"

#define PCL_LANG_USE_PERSPECTIVE      "perspective"
#define PCL_LANG_PERSPECTIVE_DISTANCE "camera distance"
#define PCL_LANG_EXPOSURE             "exposure"

#define PCL_LANG_MATERIAL "paper material"

#define PCL_LANG_FRONT_G "front Henyey-Greenstein g"
#define PCL_LANG_BACK_G  "back  Henyey-Greenstein g"
#define PCL_LANG_FRONT_G_WEIGHT "front g weight"
#define PCL_LANG_FRONT_IOR "front IOR"
#define PCL_LANG_BACK_IOR  "back IOR"
#define PCL_LANG_FRONT_ROUGH "front roughness"
#define PCL_LANG_BACK_ROUGH  "back  roughess"
#define PCL_LANG_PAPER_THICKNESS "paper thickness"
#define PCL_LANG_SIGMA_S "sigma_s"
#define PCL_LANG_SIGMA_A "sigma_a"
#define PCL_LANG_DIF_ALBEDO "diffusion albedo"

#define PCL_LANG_SPP "sampling"

#define PCL_LANG_GPU_PERFORMANCE "GPU performance"
#define PCL_LANG_RENDER_QUALITY  "render quality"

#else

#define PCL_LANG_FILE_NOT_SPECIFIED u8"警告：未指定文件名"
#define PCL_LANG_FAILED_TO_LOAD     u8"错误：无法加载指定文件"
#define PCL_LANG_UNMATCHED_SIZE     u8"错误：图像大小不匹配"
#define PCL_LANG_SUCCESS_TO_LOAD    u8"文件加载成功"

#define PCL_LANG_ADD_LAYER     u8"添加纸张"
#define PCL_LANG_LOAD_ALL      u8"读取所有"
#define PCL_LANG_LOAD_ALL_TIPS u8"删除已有纸张，并选取多个文件作为新纸张"

#define PCL_LANG_RENAME u8"重命名"
#define PCL_LANG_DELETE u8"删除"
#define PCL_LANG_BROWSE u8"浏览"

#define PCL_LANG_DELETE_WINDOW   u8"确认删除？"

#define PCL_LANG_OK     u8"确定"
#define PCL_LANG_CANCEL u8"取消"

#define PCL_LANG_NEW_LAYER_NAME u8"新纸张名"

#define PCL_LANG_BROWSE_LIGHT    u8"浏览光源文件"
#define PCL_LANG_LIGHT_INTENSITY u8"光源亮度"
#define PCL_LANG_ENV_LIGHT       u8"环境光"
#define PCL_LANG_LIGHT_DISTANCE  u8"光源间距（mm）"

#define PCL_LANG_PAPER_HORI_SIZE u8"纸张水平尺寸（mm）"
#define PCL_LANG_PAPER_DISTANCE  u8"纸张间距（mm）"

#define PCL_LANG_USE_PERSPECTIVE      u8"使用透视摄像机"
#define PCL_LANG_PERSPECTIVE_DISTANCE u8"透视投影距离"
#define PCL_LANG_EXPOSURE             u8"曝光度"

#define PCL_LANG_MATERIAL u8"纸张材质"

#define PCL_LANG_FRONT_G         u8"正面相函数不对称性"
#define PCL_LANG_BACK_G          u8"背面相函数不对称性"
#define PCL_LANG_FRONT_G_WEIGHT  u8"正面不对称性权重"
#define PCL_LANG_FRONT_IOR       u8"正面折射率"
#define PCL_LANG_BACK_IOR        u8"背面折射率"
#define PCL_LANG_FRONT_ROUGH     u8"正面粗糙度"
#define PCL_LANG_BACK_ROUGH      u8"背面粗糙度"
#define PCL_LANG_PAPER_THICKNESS u8"纸张厚度"
#define PCL_LANG_SIGMA_S         u8"内部散射率"
#define PCL_LANG_SIGMA_A         u8"内部吸收率"
#define PCL_LANG_DIF_ALBEDO      u8"Diffusion Albedo"

#define PCL_LANG_SPP u8"采样设置"

#define PCL_LANG_GPU_PERFORMANCE u8"GPU性能"
#define PCL_LANG_RENDER_QUALITY  u8"绘制质量"

#endif
