// ****************************************************************************
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

// ******************************************************************************

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Drawing;

namespace PhotoChat {
internal enum TextType {
    Error = 0,
    Remote = 1,
    Status = 2,
    System = 3,
    Me = 0x10
}

internal struct TextDescriptor {
    internal int StartPos;
    internal int Length;
    internal TextType TypeText;
    internal bool Bold;
    internal TextDescriptor(TextType typeText, int start, int len, bool bold)
    {
        StartPos = start;
        Length = len;
        TypeText = typeText;
        Bold = bold;
    }
}

internal class TextChunk {
    internal TextDescriptor Attributes;
    internal string Text;

    internal TextChunk(string text, int start, TextType originator, bool bold)
    {
        Attributes = new TextDescriptor(originator, start, text.Length, bold);
        Text = text;
    }
}

internal class RichTextBuffer {
    private RichTextBox _control;
    private ArrayList _contents;
    private Queue<TextChunk> _deferred;
    internal int InsertionPoint = 0;

    internal RichTextBuffer(RichTextBox owner)
    {
        _control = owner;
        _control.ShowSelectionMargin = true;
        _control.ScrollBars = RichTextBoxScrollBars.ForcedVertical;
        _contents = new ArrayList();
        _deferred = new Queue<TextChunk>();
        InsertionPoint = 0;
    }

    internal void AddDeferred(string text, string tag, TextType type)
    {
        lock (_deferred)
        {
            if (tag != null&& tag.Length > 0) {
                tag += ": ";
                _deferred.Enqueue(new TextChunk(tag, InsertionPoint, type, true));
                InsertionPoint += tag.Length;
            }
            if (type != TextType.Remote)
                text += '\n';
            _deferred.Enqueue(new TextChunk(text, InsertionPoint, type, false));
            InsertionPoint += text.Length;
        }
    }

    internal void ShowDeferredTexts()
    {
        if (_deferred.Count == 0)
            return;
        lock (_deferred)
        {
            foreach (TextChunk t in _deferred) {
                _contents.Add(t);
                updateControl(t);
            }
            _deferred.Clear();
        }
    }

    internal void Add(string text, string tag, TextType type)
    {
        ShowDeferredTexts();
        if (tag != null&& tag.Length > 0) {
            tag += ": ";
            _contents.Add(new TextChunk(tag, InsertionPoint, type, true));
            InsertionPoint += tag.Length;
            updateControl((TextChunk)_contents[_contents.Count - 1]);
        }
        if (type != TextType.Remote)
            text += '\n';
        //            else
        //            {
        //                MessageBox.Show(text);
        //            }
        _contents.Add(new TextChunk(text, InsertionPoint, type, false));
        InsertionPoint += text.Length;
        updateControl((TextChunk)_contents[_contents.Count - 1]);
    }

    private void updateControl(TextChunk chunk)
    {
        _control.AppendText(chunk.Text);
        _control.Select(chunk.Attributes.StartPos, chunk.Attributes.Length);
        switch (chunk.Attributes.TypeText) {
        case TextType.Error:
            _control.SelectionColor = Color.Red;
            break;

        case TextType.Status:
            _control.SelectionColor = Color.Black;
            break;

        case TextType.Remote:
            _control.SelectionColor = Color.Green;
            break;

        case TextType.Me:
            _control.SelectionColor = Color.Blue;
            break;

        default:
            _control.SelectionColor = Color.Black;
            break;
        }
        Font font = _control.SelectionFont;
        if (chunk.Attributes.Bold)
            _control.SelectionFont = new Font(font.FontFamily, font.Size, FontStyle.Bold);
        else
            _control.SelectionFont = new Font(font.FontFamily, font.Size, FontStyle.Regular);
        _control.Select(_control.Text.Length - 1, 1);
        _control.ScrollToCaret();
        _control.Update();
    }
}
}