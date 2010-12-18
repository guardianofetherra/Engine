#include "TransformComponent.h"

using namespace Helium;
using namespace Helium::Asset;

REFLECT_DEFINE_CLASS( TransformComponent );

void TransformComponent::AcceptCompositeVisitor( Reflect::Composite& comp )
{
    Reflect::Field* fieldTranslation = comp.AddField( &TransformComponent::m_Translation, "m_Translation" );
    Reflect::Field* fieldRotation = comp.AddField( &TransformComponent::m_Rotation, "m_Rotation" );
    Reflect::Field* fieldScale = comp.AddField( &TransformComponent::m_Scale, "m_Scale" );
}

Component::ComponentUsage TransformComponent::GetComponentUsage() const
{
    return Component::ComponentUsages::Overridable;
}


const Vector3& TransformComponent::GetTranslation() const
{
    return m_Translation;
}

const Vector3& TransformComponent::GetRotation() const
{
    return m_Rotation;
}

const Vector3& TransformComponent::GetScale() const
{
    return m_Scale;
}

void TransformComponent::SetTranslation( const Vector3& translation )
{
    m_Translation = translation;
}

void TransformComponent::SetRotation( const Vector3& rotation )
{
    m_Rotation = rotation;
}

void TransformComponent::SetScale( const Vector3& scale )
{
    m_Scale = scale;
}
