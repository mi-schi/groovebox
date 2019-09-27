volume_default = 0
high_pass_filter_default = 20000
low_pass_filter_default = 20


def calculate_volume(potentiometer_value: int):
    if potentiometer_value <= 500:
        return remap(potentiometer_value, 0, 500, -100, 0)

    if potentiometer_value >= 520:
        return remap(potentiometer_value, 520, 1023, 0, 10)

    return volume_default


def is_high_pass_filter(potentiometer_value: int):
    if potentiometer_value >= 520:
        return True
    return False


def calculate_high_pass_filter(potentiometer_value: int):
    if potentiometer_value >= 520:
        return remap(potentiometer_value, 520, 1023, low_pass_filter_default, high_pass_filter_default)

    return high_pass_filter_default


def calculate_low_pass_filter(potentiometer_value: int):
    if potentiometer_value <= 500:
        return remap(potentiometer_value, 0, 500, low_pass_filter_default, high_pass_filter_default)

    return low_pass_filter_default


def remap(x, in_min, in_max, out_min, out_max):
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min