#pragma once

#include "Editor/API.h"

#include "Pipeline/Content/Nodes/Curve.h"
#include "Curve.h"
#include "Tool.h"

namespace Editor
{
  class PickVisitor;
  class TranslateManipulator;

  namespace CurveEditModes
  {
    enum CurveEditMode
    {
      None,
      Modify,
      Insert,
      Remove,
    };
  }
  typedef CurveEditModes::CurveEditMode CurveEditMode;

  class CurveEditTool: public Tool
  {
  private:
    static CurveEditMode s_EditMode;
    static bool s_CurrentSelection;

    CurveEditMode m_HotEditMode;
    Editor::TranslateManipulator* m_ControlPointManipulator;
 
    //
    // RTTI
    //

    LUNA_DECLARE_TYPE(Editor::CurveEditTool, Tool);
    static void InitializeType();
    static void CleanupType();

  public:
    CurveEditTool( Editor::Scene* scene, PropertiesGenerator* generator );
    virtual ~CurveEditTool();

    CurveEditMode GetEditMode() const;

    virtual bool MouseDown( wxMouseEvent& e ) HELIUM_OVERRIDE;
    virtual void MouseUp( wxMouseEvent& e) HELIUM_OVERRIDE;
    virtual void MouseMove( wxMouseEvent& e ) HELIUM_OVERRIDE;

    virtual void KeyPress( wxKeyEvent& e ) HELIUM_OVERRIDE;
    virtual void KeyDown( wxKeyEvent& e ) HELIUM_OVERRIDE;
    virtual void KeyUp( wxKeyEvent& e ) HELIUM_OVERRIDE;

    virtual bool ValidateSelection( OS_SelectableDumbPtr& items ) HELIUM_OVERRIDE;

    virtual void Evaluate() HELIUM_OVERRIDE;
    virtual void Draw( DrawArgs* args ) HELIUM_OVERRIDE;

    virtual void CreateProperties() HELIUM_OVERRIDE;

    int GetCurveEditMode() const;
    void SetCurveEditMode( int mode );
    
    bool GetSelectionMode() const;
    void SetSelectionMode( bool mode );
    
    void StoreSelectedCurves();
    
  protected:
    OS_SelectableDumbPtr m_SelectedCurves;
  };
}