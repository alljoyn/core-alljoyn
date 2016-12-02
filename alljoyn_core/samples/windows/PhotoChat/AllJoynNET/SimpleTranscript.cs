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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace AllJoynNET {
public partial class SimpleTranscript : Form {
    public SimpleTranscript()
    {
        InitializeComponent();
    }

    public void AddText(string text)
    {
        txtTranscript.Text += text;
        txtTranscript.Select(txtTranscript.Text.Length - 2, 1);
        txtTranscript.ScrollToCaret();
    }
}
}