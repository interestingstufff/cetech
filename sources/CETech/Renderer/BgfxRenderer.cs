﻿using System;
using CELib.Window;
using SharpBgfx;

namespace CETech.Renderer
{
    public static class BgfxRenderer
    {

        private struct Data
        {
            public int ResizeW;
            public int ResizeH;
            public bool NeedResize;
        };

        private static Data _data;

        public static void Init(IWindow window, RendererBackend type)
        {
            Bgfx.SetWindowHandle(window.NativeWindowPtr);
            Bgfx.Init();
            Bgfx.SetDebugFeatures(DebugFeatures.DisplayStatistics);

            Resize(window.Width, window.Height);
        }

        public static void BeginFrame()
        {

            if (_data.NeedResize) {
                Bgfx.Reset(_data.ResizeW, _data.ResizeH);
                Bgfx.SetViewRect(0, 0, 0, _data.ResizeW, _data.ResizeH);
                _data.NeedResize = false;
            }

            Bgfx.SetDebugFeatures(DebugFeatures.DisplayStatistics | DebugFeatures.DisplayText);
            Bgfx.SetViewClear(
                0
                , ClearTargets.Color | ClearTargets.Depth
                , 0x66CCFFff);

            Bgfx.Touch(0);

            Bgfx.DebugTextClear();

            Bgfx.Submit(0, SharpBgfx.Program.Invalid);
        }

        public static void EndFrame()
        {
            Bgfx.Frame();
        }

        public static void Resize(int width, int height)
        {
            _data.NeedResize = true;
            _data.ResizeW = width;
            _data.ResizeH = height;
        }
    }
}