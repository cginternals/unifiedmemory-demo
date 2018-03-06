#version 450

layout (location=0) in float in_value;

uniform int u_sensorCount;
uniform int u_sensor;

layout (std430, binding = 0) buffer Minimums
{
    float minimums[];
};

layout (std430, binding = 1) buffer Maximums
{
    float maximums[];
};

const float plotAreaFactor = 0.95;
const float plotAreaOffset = (1.0 - plotAreaFactor) / 0.5;

void main()
{
    float minValue = minimums[u_sensor];
    float maxValue = maximums[u_sensor];
    float top = float(u_sensor+1.0-plotAreaOffset) / float(u_sensorCount);
    float bottom = float(u_sensor+plotAreaOffset) / float(u_sensorCount);
    
    float x = 0.0;
    float y = bottom + (top - bottom) * (in_value - minValue) / (maxValue - minValue);
    
    gl_Position = vec4(
        x * 2.0 - 1.0,
        y * 2.0 - 1.0,
        0.5, 1.0);
}
