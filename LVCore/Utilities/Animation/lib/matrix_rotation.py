import numpy as np

def rotation_matrix_from_euler(euler_angles, order="xyz", degrees=False):
    """
    Compute a rotation matrix from Euler angles.

    Parameters:
    - euler_angles: array-like of shape (3,)
      The three Euler angles (in radians) corresponding to the given axis order.
    - order: str, optional (default: "xyz")
      The order of axes for rotations. Supported values: 'xyz', 'zyx', etc.
    - degrees: boolean (default: "False")
      If true input angles should be converted from degrees to radians

    Returns:
    - rotation_matrix: np.ndarray of shape (3, 3)
      The resulting rotation matrix.
    """
    if degrees:
        # Convert degreess to radians
        euler_angles = np.radians(euler_angles)

    # Validate inputs
    if len(euler_angles) != 3:
        raise ValueError("euler_angles must have a length of 3.")
    if not isinstance(order, str) or len(order) != 3:
        raise ValueError("order must be a permutation of 'xyz' (e.g., 'xyz', 'zyx').")

    # Rotation matrices for individual axes
    def rotation_matrix_x(theta):
        return np.array([
            [1, 0, 0],
            [0, np.cos(theta), -np.sin(theta)],
            [0, np.sin(theta), np.cos(theta)]
        ])

    def rotation_matrix_y(theta):
        return np.array([
            [np.cos(theta), 0, np.sin(theta)],
            [0, 1, 0],
            [-np.sin(theta), 0, np.cos(theta)]
        ])

    def rotation_matrix_z(theta):
        return np.array([
            [np.cos(theta), -np.sin(theta), 0],
            [np.sin(theta), np.cos(theta), 0],
            [0, 0, 1]
        ])

    # Map axes to their respective functions
    axis_to_matrix = {
        "x": rotation_matrix_x,
        "y": rotation_matrix_y,
        "z": rotation_matrix_z
    }

    # Compute the composite rotation matrix
    rotation_matrix = np.eye(3)
    for angle, axis in zip(euler_angles, order.lower()):
        rotation_matrix = rotation_matrix @ axis_to_matrix[axis](angle)

    return rotation_matrix


def rotation_matrix_from_rotvec(rotvec):
    """
    Compute a rotation matrix from a rotation vector specified in degrees.

    Parameters:
    - rotvec: array-like of shape (3,)
      The rotation vector where the magnitude is the rotation angle in degrees,
      and the direction is the axis of rotation.

    Returns:
    - rotation_matrix: np.ndarray of shape (3, 3)
      The resulting rotation matrix.
    """
    # Convert rotation vector angle from degrees to radians
    angle_radians = np.linalg.norm(rotvec)

    # Normalize the rotation vector to get the axis of rotation
    if angle_radians == 0:
        return np.eye(3)  # If there's no rotation, return the identity matrix
    axis = rotvec / angle_radians

    # Compute components for the rotation matrix
    x, y, z = axis
    c = np.cos(angle_radians)
    s = np.sin(angle_radians)
    t = 1 - c

    # Construct the rotation matrix using the Rodrigues' rotation formula
    rotation_matrix = np.array([
        [t*x*x + c,    t*x*y - z*s,  t*x*z + y*s],
        [t*y*x + z*s,  t*y*y + c,    t*y*z - x*s],
        [t*z*x - y*s,  t*z*y + x*s,  t*z*z + c]
    ])

    return rotation_matrix

def rotation_matrix_to_rotvec(rotation_matrix):
    """
    Convert a 3x3 rotation matrix to a rotation vector (rotvec).

    Parameters:
    - rotation_matrix: np.ndarray of shape (3, 3)
      The input rotation matrix.

    Returns:
    - rotvec: np.ndarray of shape (3,)
      The equivalent rotation vector.
    """
    # Ensure the input is a valid rotation matrix
    if not (rotation_matrix.shape == (3, 3)):
        raise ValueError("Input must be a 3x3 matrix.")

    # Check if the matrix is orthogonal and has determinant close to 1
    if not (np.allclose(np.dot(rotation_matrix.T, rotation_matrix), np.eye(3)) and
            np.isclose(np.linalg.det(rotation_matrix), 1.0)):
        raise ValueError("Input must be a valid rotation matrix.")

    # Compute the angle of rotation (theta)
    trace = np.trace(rotation_matrix)
    theta = np.arccos(np.clip((trace - 1) / 2, -1.0, 1.0))  # Clip for numerical stability

    # If theta is very small, the rotation is negligible
    if np.isclose(theta, 0.0):
        return np.array([0.0, 0.0, 0.0])

    # Compute the rotation axis
    R = rotation_matrix
    axis = np.array([
        R[2, 1] - R[1, 2],
        R[0, 2] - R[2, 0],
        R[1, 0] - R[0, 1]
    ]) / (2 * np.sin(theta))

    # Compute the rotation vector
    rotvec = theta * axis
    return rotvec
