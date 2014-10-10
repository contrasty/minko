/*
Copyright (c) 2014 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "minko/component/OculusVRCamera.hpp"
#include "minko/scene/Node.hpp"
#include "minko/scene/NodeSet.hpp"
#include "minko/component/AbstractComponent.hpp"
#include "minko/render/AbstractContext.hpp"
#include "minko/component/SceneManager.hpp"
#include "minko/component/Renderer.hpp"
#include "minko/component/PerspectiveCamera.hpp"
#include "minko/component/Transform.hpp"
#include "minko/component/Surface.hpp"
#include "minko/geometry/QuadGeometry.hpp"
#include "minko/data/StructureProvider.hpp"
#include "minko/render/Texture.hpp"
#include "minko/file/AssetLibrary.hpp"
#include "minko/math/Matrix4x4.hpp"
#include "minko/render/Effect.hpp"
#include "minko/material/Material.hpp"
#include "minko/file/Loader.hpp"

#include "minko/oculus/NativeOculus.hpp"
//#include "minko/oculus/WebVROculus.hpp"

using namespace minko;
using namespace minko::scene;
using namespace minko::component;
using namespace minko::math;
using namespace minko::oculus;

OculusVRCamera::OculusVRCamera(int viewportWidth, int viewportHeight, float zNear, float zFar) :
    _aspectRatio((float)viewportWidth / (float)viewportHeight),
    _zNear(zNear),
    _zFar(zFar),
    _eyePosition(Vector3::create(0.0f, 0.0f, 0.0f)),
    _eyeOrientation(Matrix4x4::create()),
    _sceneManager(nullptr),
    _leftCameraNode(nullptr),
    _leftRenderer(nullptr),
    _rightCameraNode(nullptr),
    _rightRenderer(nullptr),
    _ppRenderer(Renderer::create()),
    _targetAddedSlot(nullptr),
    _targetRemovedSlot(nullptr),
    _addedSlot(nullptr),
    _removedSlot(nullptr),
    _renderEndSlot(nullptr),
    _oculusImpl(nullptr)
{
    _uvScaleOffset[0].first = math::Vector2::create();
    _uvScaleOffset[0].second = math::Vector2::create();
    _uvScaleOffset[1].first = math::Vector2::create();
    _uvScaleOffset[1].second = math::Vector2::create();

    updateViewport(viewportWidth, viewportHeight);
}

OculusVRCamera::~OculusVRCamera()
{
    resetOVRDevice();

    _oculusImpl->destroy();
}

void
OculusVRCamera::initialize()
{
#ifdef EMSCRIPTEN
    //_oculusImpl = WebVROculus::create();
#else
    _oculusImpl = NativeOculus::create();
#endif

    _targetAddedSlot = targetAdded()->connect(std::bind(
        &OculusVRCamera::targetAddedHandler,
        std::static_pointer_cast<OculusVRCamera>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2
    ));

    _targetRemovedSlot = targetRemoved()->connect(std::bind(
        &OculusVRCamera::targetRemovedHandler,
        std::static_pointer_cast<OculusVRCamera>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2
    ));

    initializeOVRDevice();
}

void
OculusVRCamera::updateViewport(int viewportWidth, int viewportHeight)
{
    _aspectRatio = (float)viewportWidth / (float)viewportHeight;
    _ppRenderer->viewport(0, 0, viewportWidth, viewportHeight);
    
    /*if (_leftCameraNode)
        _leftCameraNode->component<PerspectiveCamera>()->aspectRatio(_aspectRatio);
    if (_rightCameraNode)
        _rightCameraNode->component<PerspectiveCamera>()->aspectRatio(_aspectRatio);*/
}

void
OculusVRCamera::targetAddedHandler(AbsCmpPtr component, NodePtr target)
{
    if (targets().size() > 1)
        throw std::logic_error("The OculusVRCamera component cannot have more than 1 target.");

    _addedSlot = target->added()->connect(std::bind(
        &OculusVRCamera::findSceneManager,
        std::static_pointer_cast<OculusVRCamera>(shared_from_this())
    ));

    _removedSlot = target->removed()->connect(std::bind(
        &OculusVRCamera::findSceneManager,
        std::static_pointer_cast<OculusVRCamera>(shared_from_this())
    ));

    initializeCameras();

    findSceneManager();
}

void
OculusVRCamera::targetRemovedHandler(AbsCmpPtr component, NodePtr target)
{
    findSceneManager();
}

void
OculusVRCamera::resetOVRDevice()
{
    //_ovrHMDDevice = nullptr;
    //_ovrSensorDevice = nullptr;
    //_ovrSensorFusion = nullptr;
}

void
OculusVRCamera::initializeOVRDevice()
{
    // Create renderer for each eye
    _leftRenderer = Renderer::create();
    _rightRenderer = Renderer::create();
    _rightRenderer->clearBeforeRender(false);

    _oculusImpl->initializeOVRDevice(
        _leftRenderer, 
        _rightRenderer, 
        _renderTargetWidth, 
        _renderTargetHeight, 
        _uvScaleOffset
    );

    // Store default eye FOV
    _defaultLeftEyeFov = _oculusImpl->getDefaultLeftEyeFov();
    _defaultRightEyeFov = _oculusImpl->getDefaultRightEyeFov();
}

void
OculusVRCamera::initializeCameras()
{
    auto aspectRatio = (float)_renderTargetWidth / (float)_renderTargetHeight;

    auto leftCamera = PerspectiveCamera::create(
        aspectRatio,
        atan(_defaultLeftEyeFov.LeftTan + _defaultLeftEyeFov.RightTan),
        _zNear,
        _zFar
    );
    _leftCameraNode = scene::Node::create("oculusLeftEye")
        ->addComponent(Transform::create())
        ->addComponent(leftCamera)
        ->addComponent(_leftRenderer);
    targets()[0]->addChild(_leftCameraNode);

    auto rightCamera = PerspectiveCamera::create(
        aspectRatio,
        atan(_defaultRightEyeFov.LeftTan + _defaultRightEyeFov.RightTan),
        _zNear,
        _zFar
    );
    _rightCameraNode = scene::Node::create("oculusRightEye")
        ->addComponent(Transform::create())
        ->addComponent(rightCamera)
        ->addComponent(_rightRenderer);
    targets()[0]->addChild(_rightCameraNode);
}

std::array<std::shared_ptr<geometry::Geometry>, 2>
OculusVRCamera::createDistortionGeometry(std::shared_ptr<render::AbstractContext> context)
{
    return _oculusImpl->createDistortionGeometry(context);
}

void
OculusVRCamera::initializePostProcessingRenderer()
{
    auto geometries = createDistortionGeometry(_sceneManager->assets()->context());
    auto loader = file::Loader::create(_sceneManager->assets()->loader());
    
    loader->queue("effect/OculusVR/OculusVR.effect");

    auto complete = loader->complete()->connect([&](file::Loader::Ptr loader)
    {
        auto effect = _sceneManager->assets()->effect("effect/OculusVR/OculusVR.effect");

        auto materialLeftEye = material::Material::create();
        materialLeftEye->set("eyeToSourceUVScale", _uvScaleOffset[0].first);
        materialLeftEye->set("eyeToSourceUVOffset", _uvScaleOffset[0].second);
        materialLeftEye->set("eyeRotationStart", math::Matrix4x4::create());
        materialLeftEye->set("eyeRotationEnd", math::Matrix4x4::create());
        materialLeftEye->set("texture", _renderTarget);

        auto materialRightEye = material::Material::create();
        materialRightEye->set("eyeToSourceUVScale", _uvScaleOffset[1].first);
        materialRightEye->set("eyeToSourceUVOffset", _uvScaleOffset[1].second);
        materialRightEye->set("eyeRotationStart", math::Matrix4x4::create());
        materialRightEye->set("eyeRotationEnd", math::Matrix4x4::create());
        materialRightEye->set("texture", _renderTarget);

        _ppScene = scene::Node::create()
            ->addComponent(_ppRenderer)
            ->addComponent(Surface::create(geometries[0], materialLeftEye, effect))
            ->addComponent(Surface::create(geometries[1], materialRightEye, effect));
    });

    loader->load();
}

void
OculusVRCamera::findSceneManager()
{
    NodeSet::Ptr roots = NodeSet::create(targets())
        ->roots()
        ->where([](NodePtr node)
        {
            return node->hasComponent<SceneManager>();
        });

    if (roots->nodes().size() > 1)
        throw std::logic_error("OculusVRCamera cannot be in two separate scenes.");
    else if (roots->nodes().size() == 1)
        setSceneManager(roots->nodes()[0]->component<SceneManager>());
    else
        setSceneManager(nullptr);
}

void
OculusVRCamera::setSceneManager(SceneManager::Ptr sceneManager)
{
    if (_sceneManager == sceneManager)
        return;

    _sceneManager = sceneManager;

    auto context = sceneManager->assets()->context();

    _renderTarget = render::Texture::create(context, _renderTargetWidth, _renderTargetHeight, false, true);
    _renderTarget->upload();
    _leftRenderer->target(_renderTarget);
    _rightRenderer->target(_renderTarget);

    _renderEndSlot = sceneManager->renderingEnd()->connect(std::bind(
        &OculusVRCamera::renderEndHandler,
        std::static_pointer_cast<OculusVRCamera>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    ));

    _renderBeginSlot = sceneManager->renderingBegin()->connect(std::bind(
        &OculusVRCamera::updateCameraOrientation,
        std::static_pointer_cast<OculusVRCamera>(shared_from_this())
    ), 1000.f);

    initializePostProcessingRenderer();
}

void
OculusVRCamera::renderEndHandler(std::shared_ptr<SceneManager>   sceneManager,
                                 uint                            frameId,
                                 render::AbstractTexture::Ptr    renderTarget)
{
    _ppRenderer->render(sceneManager->assets()->context());
}

void
OculusVRCamera::updateCameraOrientation()
{
    std::array<Matrix4x4::Ptr, 2> viewMatrixes = {
        _leftCameraNode->component<Transform>()->matrix(),
        _rightCameraNode->component<Transform>()->matrix()
    };

    _oculusImpl->updateCameraOrientation(viewMatrixes, _ppScene);
}