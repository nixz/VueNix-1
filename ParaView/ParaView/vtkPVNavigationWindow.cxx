/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVNavigationWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVNavigationWindow.h"

#include "vtkKWApplication.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

#include <stdarg.h>

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVNavigationWindow );

vtkPVNavigationWindow::vtkPVNavigationWindow()
{
  this->Width     = -1;
  this->Height    = -1;
  this->Canvas    = vtkKWWidget::New();
  this->ScrollBar = vtkKWWidget::New();
  this->PopupMenu = vtkKWMenu::New();
}

vtkPVNavigationWindow::~vtkPVNavigationWindow()
{
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }
  if (this->ScrollBar)
    {
    this->ScrollBar->Delete();
    }
  if ( this->PopupMenu )
    {
    this->PopupMenu->Delete();
    }
}


void vtkPVNavigationWindow::CalculateBBox(vtkKWWidget* canvas, char* name, 
					  int bbox[4])
{
  char *result;

  // Get the bounding box for the name. We may need to highlight it.
  this->Script("%s bbox %s", canvas->GetWidgetName(), name);
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);

}

char* vtkPVNavigationWindow::CreateCanvasItem(const char *format, ...)
{
  char event[16000];
  char* result, *retVal;
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  this->Script(event);
  result = this->Application->GetMainInterp()->result;
  retVal = new char[strlen(result)+1];
  strcpy(retVal,result);
  return retVal;
}

void vtkPVNavigationWindow::Update(vtkPVSource *currentSource)
{
  vtkPVSource *source;
  vtkPVData **inputs = currentSource->GetPVInputs();
  vtkPVData **outputs;
  int numInputs, xMid, yMid=0, y, i;
  char *tmp;
  int bbox[4];
  int bboxIn[4], bboxOut[4], bboxSource[4];
  vtkPVData *moreOut;
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";  
  
  this->Script("pack forget %s", this->ScrollBar->GetWidgetName());

  // Clear the canvas
  this->Script("%s delete all", this->Canvas->GetWidgetName());

  // Draw the name of the assembly.
  tmp = this->CreateCanvasItem("%s create text %d %d -text {%s} -font %s "
			       "-tags x",
			       this->Canvas->GetWidgetName(), 170, 10, 
			       currentSource->GetName(),
			       font);
  this->Script("%s bind %s <ButtonPress-3> "
	       "{ %s DisplayModulePopupMenu %s %%X %%Y }",
	       this->Canvas->GetWidgetName(), tmp, this->GetTclName(), 
	       currentSource->GetTclName());
  // Get the bounding box for the name.
  this->CalculateBBox(this->Canvas, tmp, bboxSource);
  delete [] tmp;
  tmp = 0;

  // Put the inputs in the canvas.
  if (inputs)
    {
    y = 10;
    numInputs = currentSource->GetNumberOfPVInputs();
    // only want to set xMid once
    xMid = bboxSource[0] - 25;
    for (i = 0; i < numInputs; i++)
      {
      if (inputs[i] && (source = inputs[i]->GetPVSource()) )
        {
        // Draw the name of the assembly.
        tmp = this->CreateCanvasItem(
          "%s create text %d %d -text {%s} -font %s -anchor e "
	  "-tags x -fill blue",
          this->Canvas->GetWidgetName(), bboxSource[0]-50, y,
          source->GetName(), font);
        
	this->CalculateBBox(this->Canvas, tmp, bboxIn);
        this->Script("%s bind %s <ButtonPress-1> {%s SetCurrentPVSourceCallback %s}",
                     this->Canvas->GetWidgetName(), tmp,
                     currentSource->GetPVWindow()->GetTclName(), 
		     source->GetTclName());
	this->Script("%s bind %s <Enter> {%s HighlightObject %s 1}",
		     this->Canvas->GetWidgetName(), tmp,
		     this->GetTclName(), tmp);
	this->Script("%s bind %s <Leave> {%s HighlightObject %s 0}",
		     this->Canvas->GetWidgetName(), tmp,
		     this->GetTclName(), tmp);
	this->Script("%s bind %s <ButtonPress-3> "
		     "{ %s DisplayModulePopupMenu %s %%X %%Y }",
		     this->Canvas->GetWidgetName(), tmp, this->GetTclName(), 
		     source->GetTclName());
        
        delete [] tmp;
        tmp = 0;

	// only want to set yMid once
	if ( i == 0 )
	  {
	  yMid = static_cast<int>(0.5 * (bboxIn[1]+bboxIn[3]));
	  }

        // Draw a line from input to source.
        if (y == 10)
          {
          tmp = this->CreateCanvasItem(
	    "%s create line %d %d %d %d -fill gray50 -arrow last",
	    this->Canvas->GetWidgetName(), bboxIn[2], yMid,
	    bboxSource[0], yMid);
	  delete[] tmp;
          }
        else
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->Canvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->Canvas->GetWidgetName(), bboxIn[2],
                       yMid, xMid, yMid);
          }
        
        if (source->GetPVInputs())
          {
	  if (source->GetPVInput()->GetPVSource())
	    {
	    // Draw ellipsis indicating that this source has a source.
	    this->Script("%s create line %d %d %d %d",
			 this->Canvas->GetWidgetName(), bboxIn[0] - 18, yMid, 
			 bboxIn[0] - 16, yMid);
	    this->Script("%s create line %d %d %d %d",
			 this->Canvas->GetWidgetName(), bboxIn[0] - 14, yMid, 
			 bboxIn[0] - 12, yMid);
	    this->Script("%s create line %d %d %d %d",
			 this->Canvas->GetWidgetName(), bboxIn[0] - 10, yMid, 
			 bboxIn[0] - 8, yMid);
	    }
	  }
	}
      y += 15;
      }
    }

  // Put the outputs in the canvas.
  outputs = currentSource->GetPVOutputs();
  int numOutputs = currentSource->GetNumberOfPVOutputs();
  if (outputs)
    {
    y = 10;
    if (outputs[0])
      {
      // only want to set xMid  once
      xMid = bboxSource[2] + 25;
      // We have two display modes:
      // 1. If the number of outputs is > 1, we display the first
      // consumer of each output (with the current implementation,
      // there is no way of assigning more than one consumer to
      // an output from the user interface if there are more than
      // 1 outputs)
      // 2. If there is only one output, we display all the consumers
      // of that output
      int num;
      if (numOutputs > 1)
	{
	num = numOutputs;
	}
      else
	{
	num = outputs[0]->GetNumberOfPVConsumers();
	}
      for (i = 0; i < num; i++)
	{
	if (numOutputs > 1)
	  {
	  if ( outputs[i]->GetNumberOfPVConsumers() == 0 )
	    {
	    continue;
	    }
	  source = outputs[i]->GetPVConsumer(0);
	  }
	else
	  {
	  source = outputs[0]->GetPVConsumer(i);
	  }
        
        // Draw the name of the assembly .
        tmp = this->CreateCanvasItem(
          "%s create text %d %d -text {%s} -font %s -anchor w "
	  "-tags x -fill blue",
          this->Canvas->GetWidgetName(), bboxSource[2]+50, y,
          source->GetName(), font);
        
        // Get the bounding box for the name.
	this->CalculateBBox(this->Canvas, tmp, bboxOut);
        this->Script("%s bind %s <ButtonPress-1> {%s  SetCurrentPVSourceCallback %s}",
                     this->Canvas->GetWidgetName(), tmp,
                     currentSource->GetPVWindow()->GetTclName(), 
		     source->GetTclName());
	this->Script("%s bind %s <Enter> {%s HighlightObject %s 1}",
		     this->Canvas->GetWidgetName(), tmp,
		     this->GetTclName(), tmp);
	this->Script("%s bind %s <Leave> {%s HighlightObject %s 0}",
		     this->Canvas->GetWidgetName(), tmp,
		     this->GetTclName(), tmp); 
	this->Script("%s bind %s <ButtonPress-3> "
		     "{ %s DisplayModulePopupMenu %s %%X %%Y }",
		     this->Canvas->GetWidgetName(), tmp, this->GetTclName(), 
		     source->GetTclName());
        delete [] tmp;
        tmp = NULL;
        
	// only want to set yMid once
	if ( i == 0 )
	  {
	  yMid = static_cast<int>(0.5 * (bboxOut[1]+bboxOut[3]));
	  }

        // Draw to output.
        if (y == 10)
          { // first is a special case (single line).
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->Canvas->GetWidgetName(), bboxSource[2],
                       yMid, bboxOut[0], yMid);
          }
        else
          {
	  xMid = (int)(0.5 * (bboxSource[2] + bboxOut[0]));
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->Canvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->Canvas->GetWidgetName(), xMid, yMid,
                       bboxOut[0], yMid);
          }
        if ((moreOut = source->GetPVOutput(0)))
          {
          if (moreOut->GetNumberOfPVConsumers() > 0)
            {
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(),
                         bboxOut[2]+10, yMid, bboxOut[2]+12, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(),
                         bboxOut[2]+14, yMid, bboxOut[2]+16, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->Canvas->GetWidgetName(),
                         bboxOut[2]+18, yMid, bboxOut[2]+20, yMid);
            }
          }
        y += 15;
	}
      }
    }

  char all[4] = "all";
  this->CalculateBBox(this->Canvas, all, bbox);
  this->Script("winfo height %s", this->Canvas->GetWidgetName());
  int height = vtkKWObject::GetIntegerResult(this->Application);
  if ( height > 1 && (bbox[3] - bbox[1]) > height )
    {
    this->Script("pack %s -fill both -side right", 
		 this->ScrollBar->GetWidgetName());
    this->Script("%s configure -scrollregion \"%d %d %d %d\"", 
		 this->Canvas->GetWidgetName(), 
		 0, bbox[1], 341, bbox[3]);
    }
  else
    {
    this->Script("%s configure -scrollregion \"%d %d %d %d\"", 
		 this->Canvas->GetWidgetName(), 
		 0, 0, 341, 45);
    }
  
}

void vtkPVNavigationWindow::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Window already created");
    return;
    }

  this->SetApplication(app);

  ostrstream opts;
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s %s", wname, args);

  if (this->Width > 0 && this->Height > 0)
    {
    opts << " -width " << this->Width << " -height " << this->Height;
    }
  else if (this->Width > 0)
    {
    opts << " -width " << this->Width;
    }
  else if (this->Height > 0)
    {
    opts << " -height " << this->Height;
    }

  opts << " -bg white" << ends;

  char* optstr = opts.str();
  this->Canvas->SetParent(this);
  this->Canvas->Create(this->Application, "canvas", optstr); 
  delete[] optstr;

  ostrstream command;
  this->ScrollBar->SetParent(this);
  command << "-command \"" <<  this->Canvas->GetWidgetName()
	  << " yview\"" << ends;
  char* commandStr = command.str();
  this->ScrollBar->Create(this->Application, "scrollbar", commandStr);
  delete[] commandStr;

  this->Script("%s configure -yscrollcommand \"%s set\"", 
	       this->Canvas->GetWidgetName(),
	       this->ScrollBar->GetWidgetName());

  this->Script("pack %s -fill both -expand t -side left", this->Canvas->GetWidgetName());
  this->PopupMenu->SetParent(this);
  this->PopupMenu->Create(this->Application, "-tearoff 0");
  this->PopupMenu->AddCommand("Delete", this, "DeleteWidget", 0, 
			      "Delete current widget");
  char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
  this->PopupMenu->AddCheckButton("Visibility", var, this, "Visibility", 0,
				  "Set visibility for the current object");  
  delete [] var;
  /*
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  this->PopupMenu->AddCascade(
    "VTK Filters", pvApp->GetMainWindow()->GetFilterMenu(),
    4, "Choose a filter from a list of VTK filters");
  */
}

void vtkPVNavigationWindow::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Modified();
  this->Width = width;

  if (this->Application != NULL)
    {
    this->Script("%s configure -width %d", this->Canvas->GetWidgetName(), 
		 width);
    }
}

void vtkPVNavigationWindow::SetHeight(int height)
{
  if (this->Height == height)
    {
    return;
    }

  this->Modified();
  this->Height = height;

  if (this->Application != NULL)
    {
    this->Script("%s configure -height %d", this->Canvas->GetWidgetName(), 
		 height);
    }
}

void vtkPVNavigationWindow::HighlightObject(const char* widget, int onoff)
{
  this->Script("%s itemconfigure %s -fill %s", 
	       this->Canvas->GetWidgetName(), widget,
	       (onoff ? "red" : "blue") );
}

//----------------------------------------------------------------------------
void vtkPVNavigationWindow::DisplayModulePopupMenu(const char* module, 
						   int x, int y)
{
  //cout << "Popup for module: " << module << " at " << x << ", " << y << endl;
  vtkKWApplication *app = this->Application;
  ostrstream str;
  if ( app->EvaluateBooleanExpression("%s IsDeletable", module) )
    {
    str << "ExecuteCommandOnModule " << module << " DeleteCallback" << ends;
    this->PopupMenu->SetEntryCommand("Delete", this, str.str());
    this->PopupMenu->SetState("Delete", vtkKWMenu::Normal);
    }
  else
    {
    this->PopupMenu->SetState("Delete", vtkKWMenu::Disabled);
    }
  str.rdbuf()->freeze(0);
  ostrstream str1;
  if ( !app->EvaluateBooleanExpression("%s GetHideDisplayPage", module) )
    {
    this->PopupMenu->SetState("Visibility", vtkKWMenu::Normal);
    char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
    str1 << "[ " << module << " GetPVOutput ] SetVisibility $" 
	 << var << ";"
	 << "[ [ Application GetMainWindow ] GetMainView ] EventuallyRender" 
	 <<  ends;
    this->PopupMenu->SetEntryCommand("Visibility", str1.str());
    if ( app->EvaluateBooleanExpression("[ %s GetPVOutput ] GetVisibility",
					module) )
      {
      this->Script("set %s 1", var);
      }
    else
      {
      this->Script("set %s 0", var);
      }
    delete [] var;
    }
  else
    {
    this->PopupMenu->SetState("Visibility", vtkKWMenu::Disabled);
    }
  this->Script("tk_popup %s %d %d", this->PopupMenu->GetWidgetName(), x, y);
  str1.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkPVNavigationWindow::ExecuteCommandOnModule(
  const char* module, const char* command)
{
  //cout << "Executing: " << command << " on module: " << module << endl;
  this->Script("%s %s", module, command);
}

//----------------------------------------------------------------------------
void vtkPVNavigationWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Canvas: " << this->GetCanvas() << endl;
}
