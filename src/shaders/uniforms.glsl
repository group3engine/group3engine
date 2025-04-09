struct CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 inverseProjection;
    mat4 inverseView;
    vec4 cameraPosition;
    vec2 viewportSize;
    float fov;
    float nearPlane;
    float farPlane;
};
