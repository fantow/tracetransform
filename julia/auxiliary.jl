using Images

function zscore(input::Vector)
    local _mean = mean(input)
    local _std = std(input)

    output::Vector = similar(input)
    for i in 1:length(input)
        output[i] = (input[i] - _mean) / _std
    end

    return output
end

function trapz(x::Vector, y::Vector)
    @assert length(x) == length(y)

    sum = 0
    for i = 1:length(x)-1
        sum += (x[i+1] - x[i]) * (y[i+1] + y[i])
    end
    return sum * 0.5
end

mat2gray(input::Image{Float32}) = share(input,
    uint8(clamp(data(input) * 255 / maximum(data(input)), 0, 255)))

gray2mat(input::Image{Uint8}) = share(input, float32(input.data / 255))
